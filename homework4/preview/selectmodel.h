#pragma once

#include <QSqlQueryModel>

class Database;

// Read-only model over an arbitrary SELECT. The whole result set is fetched up front so
// that a QSortFilterProxyModel on top of it can filter locally, exactly like PatientTableModel.
class SelectModel final : public QSqlQueryModel
{
public:
    explicit SelectModel(const Database &database, QObject *parent = nullptr);

    bool load(const QString &statement, QString &error);

private:
    const Database &database_;
};
