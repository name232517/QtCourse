#include "patienttablemodel.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include <utility>

bool PatientTableModel::load(const QSqlDatabase &database, QString &error)
{
    error.clear();
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

QVariant PatientTableModel::data(const QModelIndex &index, int role) const
{
    const QVariant value = QSqlQueryModel::data(index, role);
    if (role != Qt::DisplayRole || !index.isValid()) {
        return value;
    }

    if (index.column() == Sex) {
        switch (value.toInt()) {
        case 1:
            return QStringLiteral("男");
        case 2:
            return QStringLiteral("女");
        default:
            return QStringLiteral("未知");
        }
    }
    if (index.column() == Mobile && value.toString().isEmpty()) {
        return QStringLiteral("—");
    }
    return value;
}
