#include "database.h"
#include "passworddigest.h"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace {
constexpr int SchemaVersion = 1;
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

QSqlDatabase Database::connection() const
{
    return db_;
}
