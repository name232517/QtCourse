#include "database.h"
#include "passworddigest.h"

#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include <cmath>

namespace {
constexpr int SchemaVersion = 1;

QString validatePatient(const PatientRecord &patient, const std::optional<PatientRecord> &original)
{
    if (patient.name.trimmed().isEmpty() || patient.name.toUcs4().size() > 64) {
        return QStringLiteral("请输入 1 至 64 个字符的姓名。");
    }
    if (patient.identityCard.trimmed().isEmpty() || patient.identityCard.toUcs4().size() > 20
        || patient.identityCard != patient.identityCard.trimmed()) {
        return QStringLiteral("请输入 1 至 20 个字符的证件号码。");
    }
    if (patient.sex < 0 || patient.sex > 2) {
        return QStringLiteral("请选择性别。");
    }
    if (!original || patient.birthDate != original->birthDate) {
        const QDate birthDate = QDate::fromString(patient.birthDate, Qt::ISODate);
        if (!birthDate.isValid() || birthDate > QDate::currentDate()) {
            return QStringLiteral("请选择有效的出生日期，且不能晚于今天。");
        }
    }
    if (patient.mobile.toUcs4().size() > 20) {
        return QStringLiteral("手机号码最多可填写 20 个字符。");
    }
    if (patient.height && (!std::isfinite(*patient.height) || *patient.height <= 0)) {
        return QStringLiteral("身高应为大于零的数字，或留空。");
    }
    if (patient.weight && (!std::isfinite(*patient.weight) || *patient.weight <= 0)) {
        return QStringLiteral("体重应为大于零的数字，或留空。");
    }
    return {};
}

void bindPatientFields(QSqlQuery &query, const PatientRecord &patient, const QString &prefix)
{
    query.bindValue(prefix + QStringLiteral("name"), patient.name);
    query.bindValue(prefix + QStringLiteral("card"), patient.identityCard);
    query.bindValue(prefix + QStringLiteral("sex"), patient.sex);
    query.bindValue(prefix + QStringLiteral("dob"), patient.birthDate);
    query.bindValue(prefix + QStringLiteral("mobile"), patient.mobile.isNull()
        ? QVariant(QMetaType::fromType<QString>()) : QVariant(patient.mobile));
    query.bindValue(prefix + QStringLiteral("height"), patient.height
        ? QVariant(*patient.height) : QVariant(QMetaType::fromType<double>()));
    query.bindValue(prefix + QStringLiteral("weight"), patient.weight
        ? QVariant(*patient.weight) : QVariant(QMetaType::fromType<double>()));
}
}

Database::Database()
    : connectionName_(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

Database::~Database()
{
    if (db_.isValid()) {
        db_.close();
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

bool Database::open(const QString &filePath, QString &error)
{
    error.clear();
    if (filePath.trimmed().isEmpty()) {
        error = QStringLiteral("请指定数据库文件路径。");
        return false;
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        error = QStringLiteral("找不到 Qt 的 QSQLITE 驱动。");
        return false;
    }

    if (!db_.isValid()) {
        db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    }
    db_.close();
    db_.setDatabaseName(filePath);
    db_.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));

    if (!db_.open()) {
        error = QStringLiteral("无法打开数据库 %1。%2").arg(filePath, db_.lastError().text());
        return false;
    }

    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        error = QStringLiteral("无法启用外键检查。%1").arg(query.lastError().text());
        query.finish();
        db_.close();
        return false;
    }

    if (!initializeSchema(error)) {
        db_.close();
        return false;
    }
    return true;
}

bool Database::initializeSchema(QString &error)
{
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("BEGIN IMMEDIATE"))) {
        error = QStringLiteral("无法开始数据库初始化事务。%1").arg(query.lastError().text());
        return false;
    }

    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        error = QStringLiteral("无法读取数据库结构版本。%1").arg(query.lastError().text());
        query.finish();
        db_.rollback();
        return false;
    }
    const int version = query.value(0).toInt();
    query.finish();

    if (version == 0) {
        QFile schema(QStringLiteral(":/sql/schema.sql"));
        if (!schema.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("无法读取建表脚本。%1").arg(schema.errorString());
            db_.rollback();
            return false;
        }
        // The bundled version-1 script contains only statements with no internal semicolons.
        const auto statements = QString::fromUtf8(schema.readAll()).split(QLatin1Char(';'));
        for (const QString &statement : statements) {
            const QString sql = statement.trimmed();
            if (!sql.isEmpty() && !query.exec(sql)) {
                error = QStringLiteral("无法创建数据库结构。%1").arg(query.lastError().text());
                db_.rollback();
                return false;
            }
        }
    } else if (version != SchemaVersion) {
        error = QStringLiteral("当前程序无法打开结构版本为 %1 的数据库。").arg(version);
        db_.rollback();
        return false;
    }

    if (!db_.commit()) {
        error = QStringLiteral("无法提交数据库初始化事务。%1").arg(db_.lastError().text());
        db_.rollback();
        return false;
    }
    return true;
}

bool Database::addDemoData(QString &error)
{
    error.clear();
    if (!db_.isOpen()) {
        error = QStringLiteral("请先打开数据库。");
        return false;
    }

    const QByteArray digest = PasswordDigest::create(QByteArrayLiteral("123456"));
    if (digest.isEmpty()) {
        error = QStringLiteral("无法生成演示账号的密码摘要。");
        return false;
    }
    if (!db_.transaction()) {
        error = QStringLiteral("无法开始演示数据事务。%1").arg(db_.lastError().text());
        return false;
    }

    QSqlQuery query(db_);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO \"User\" (ID, FULLNAME, USERNAME, PASSWORD) "
            "VALUES ('demo-admin', '实验管理员', 'admin', ?) ON CONFLICT(ID) DO NOTHING"))) {
        error = QStringLiteral("无法准备演示账号数据。%1").arg(query.lastError().text());
        db_.rollback();
        return false;
    }

    query.addBindValue(QString::fromLatin1(digest));
    if (!query.exec() || !query.exec(QStringLiteral(
            "INSERT INTO Patient (ID, ID_CARD, NAME, SEX, DOB, HEIGHT, WEIGHT, MOBILEPHONE) VALUES "
            "('demo-001', '000000200001010001', '林晓晨', 1, '2000-01-01', 172.5, 66.0, NULL), "
            "('demo-002', '000000199805140002', '陈若宁', 2, '1998-05-14', 163.0, 54.5, NULL), "
            "('demo-003', '000000200308250003', '罗雨安', 0, '2003-08-25', NULL, NULL, NULL) "
            "ON CONFLICT(ID) DO NOTHING"))) {
        error = QStringLiteral("无法写入演示数据。%1").arg(query.lastError().text());
        db_.rollback();
        return false;
    }

    if (!db_.commit()) {
        error = QStringLiteral("无法提交演示数据。%1").arg(db_.lastError().text());
        db_.rollback();
        return false;
    }
    return true;
}

Database::CredentialLookup Database::passwordDigestForUser(const QString &username, QByteArray &digest) const
{
    digest.clear();
    if (!db_.isOpen()) {
        return CredentialLookup::Error;
    }

    QSqlQuery query(db_);
    if (!query.prepare(QStringLiteral("SELECT PASSWORD FROM \"User\" WHERE USERNAME = :username"))) {
        return CredentialLookup::Error;
    }
    query.bindValue(QStringLiteral(":username"), username);
    if (!query.exec()) {
        return CredentialLookup::Error;
    }
    if (!query.next()) {
        return query.lastError().isValid() ? CredentialLookup::Error : CredentialLookup::UnknownUser;
    }

    digest = query.value(0).toByteArray();
    return CredentialLookup::Found;
}

Database::PatientRemoval Database::removePatient(const QString &id, QString &error) const
{
    error.clear();
    if (!db_.isOpen()) {
        error = QStringLiteral("数据库连接未打开。");
        return PatientRemoval::Error;
    }

    QSqlQuery query(db_);
    if (!query.prepare(QStringLiteral("DELETE FROM Patient WHERE ID = :id"))) {
        error = query.lastError().text();
        return PatientRemoval::Error;
    }
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        error = query.lastError().text();
        return PatientRemoval::Error;
    }

    const int removed = query.numRowsAffected();
    if (removed < 0) {
        error = QStringLiteral("无法确认删除结果，请重新查询。");
        return PatientRemoval::Error;
    }
    return removed == 0 ? PatientRemoval::Missing : PatientRemoval::Removed;
}

Database::PatientSave Database::savePatient(PatientRecord &patient,
                                           const std::optional<PatientRecord> &original, QString &error) const
{
    error = validatePatient(patient, original);
    if (!error.isEmpty()) {
        return PatientSave::Error;
    }
    if (!db_.isOpen()) {
        error = QStringLiteral("数据库连接未打开。");
        return PatientSave::Error;
    }
    if (original && patient.id != original->id) {
        error = QStringLiteral("已有患者的病历编号不可修改。");
        return PatientSave::Error;
    }

    const QString id = original ? original->id : QUuid::createUuid().toString(QUuid::Id128);
    const QString createdTimestamp = original ? original->createdTimestamp
        : QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery query(db_);
    // Match the original values so a form cannot overwrite changes made since it opened.
    const QString statement = original ? QStringLiteral(
        "UPDATE Patient SET NAME = :name, ID_CARD = :card, SEX = :sex, DOB = :dob, "
        "MOBILEPHONE = :mobile, HEIGHT = :height, WEIGHT = :weight WHERE ID = :id "
        "AND NAME IS :oldname AND ID_CARD COLLATE BINARY IS :oldcard AND SEX IS :oldsex "
        "AND DOB IS :olddob AND MOBILEPHONE IS :oldmobile "
        "AND HEIGHT IS :oldheight AND WEIGHT IS :oldweight AND CREATEDTIMESTAMP IS :oldcreated")
        : QStringLiteral(
        "INSERT INTO Patient (ID, NAME, ID_CARD, SEX, DOB, MOBILEPHONE, HEIGHT, WEIGHT, CREATEDTIMESTAMP) "
        "VALUES (:id, :name, :card, :sex, :dob, :mobile, :height, :weight, :created)");
    if (!query.prepare(statement)) {
        error = QStringLiteral("无法保存患者档案。%1").arg(query.lastError().text());
        return PatientSave::Error;
    }

    query.bindValue(QStringLiteral(":id"), id);
    bindPatientFields(query, patient, QStringLiteral(":"));
    if (original) {
        bindPatientFields(query, *original, QStringLiteral(":old"));
        query.bindValue(QStringLiteral(":oldcreated"), createdTimestamp);
    } else {
        query.bindValue(QStringLiteral(":created"), createdTimestamp);
    }
    if (!query.exec()) {
        const QSqlError sqlError = query.lastError();
        const QString detail = sqlError.databaseText();
        if (detail.contains(QStringLiteral("UNIQUE constraint failed: Patient.ID_CARD"))) {
            error = QStringLiteral("证件号码已存在，请核对后重试。");
        } else if (detail.contains(QStringLiteral("UNIQUE constraint failed: Patient.ID"))) {
            error = QStringLiteral("生成的病历编号已存在，请重新保存。");
        } else {
            error = QStringLiteral("无法保存患者档案。%1").arg(sqlError.text());
        }
        return PatientSave::Error;
    }
    if (original && query.numRowsAffected() == 0) {
        error = QStringLiteral("该患者记录已被更改或删除，请返回列表后重新查询。");
        return PatientSave::Stale;
    }
    patient.id = id;
    patient.createdTimestamp = createdTimestamp;
    return PatientSave::Saved;
}

QSqlDatabase Database::connection() const
{
    return db_;
}
