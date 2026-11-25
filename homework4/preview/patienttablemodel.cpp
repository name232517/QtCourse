#include "patienttablemodel.h"

#include "database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include <utility>

PatientTableModel::PatientTableModel(const Database &database, QObject *parent)
    : QSqlQueryModel(parent), database_(database)
{
}

bool PatientTableModel::load(QString &error)
{
    error.clear();
    const QSqlDatabase database = database_.connection();
    if (!database.isOpen()) {
        clear();
        error = QStringLiteral("请先打开数据库。");
        return false;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral(
        "SELECT ID, NAME, SEX, DOB, ID_CARD, MOBILEPHONE, HEIGHT, WEIGHT "
        "FROM Patient ORDER BY ID"))) {
        clear();
        error = QStringLiteral("无法读取患者记录。%1").arg(query.lastError().text());
        return false;
    }

    setQuery(std::move(query));
    // Fetch every batch so the local filter and record count cover the full result.
    while (!lastError().isValid() && canFetchMore()) {
        fetchMore();
    }
    if (lastError().isValid()) {
        error = QStringLiteral("无法读取患者记录。%1").arg(lastError().text());
        clear();
        return false;
    }

    const QStringList headers = {
        QStringLiteral("病历编号"), QStringLiteral("姓名"), QStringLiteral("性别"),
        QStringLiteral("出生日期"), QStringLiteral("证件号码"), QStringLiteral("手机号码"),
        QStringLiteral("身高"), QStringLiteral("体重")
    };
    for (int column = 0; column < headers.size(); ++column) {
        setHeaderData(column, Qt::Horizontal, headers.at(column));
    }
    return true;
}

PatientRecord PatientTableModel::patientAt(int row) const
{
    const auto value = [this, row](Column column) { return data(index(row, column), Qt::EditRole); };
    const QVariant height = value(Height);
    const QVariant weight = value(Weight);
    PatientRecord patient;
    patient.id = value(Id).toString();
    patient.name = value(Name).toString();
    patient.identityCard = value(IdentityCard).toString();
    patient.sex = value(Sex).toInt();
    patient.birthDate = value(BirthDate).toString();
    const QVariant mobile = value(Mobile);
    patient.mobile = mobile.isNull() ? QString() : mobile.toString();
    patient.height = height.isNull() ? std::nullopt : std::optional<double>(height.toDouble());
    patient.weight = weight.isNull() ? std::nullopt : std::optional<double>(weight.toDouble());
    return patient;
}

QVariant PatientTableModel::data(const QModelIndex &index, int role) const
{
    const QVariant value = QSqlQueryModel::data(index, role);
    if (role != Qt::DisplayRole || !index.isValid()) {
        return value;
    }

    if (index.column() == Sex) {
        return PatientRecord::sexLabel(value.toInt());
    }
    if (index.column() == Mobile && value.toString().isEmpty()) {
        return QStringLiteral("—");
    }
    return value;
}

Qt::ItemFlags PatientTableModel::flags(const QModelIndex &index) const
{
    auto result = QSqlQueryModel::flags(index);
    if (index.isValid() && index.column() == Sex) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

bool PatientTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != Sex || role != Qt::EditRole) {
        return false;
    }
    bool numeric = false;
    const int sex = value.toInt(&numeric);
    if (!numeric || sex < 0 || sex > 2) {
        return false;
    }

    const PatientRecord original = patientAt(index.row());
    if (original.sex == sex) {
        return true;
    }
    PatientRecord updated = original;
    updated.sex = sex;
    QString error;
    if (database_.savePatient(updated, original, error) != Database::PatientSave::Saved) {
        emit patientSaveFailed(error);
        return false;
    }

    emit patientSaved(updated.id);
    return true;
}
