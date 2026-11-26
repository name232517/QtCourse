#include "selectmodel.h"

#include "database.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

SelectModel::SelectModel(const Database &database, QObject *parent)
    : QSqlQueryModel(parent), database_(database)
{
}

bool SelectModel::load(const QString &statement, QString &error)
{
    error.clear();
    QSqlQuery query(database_.connection());
    if (!query.exec(statement)) {
        error = query.lastError().text();
        clear();
        return false;
    }

    setQuery(std::move(query));
    if (lastError().isValid()) {
        error = lastError().text();
        clear();
        return false;
    }

    // The proxy filters locally, so every row has to be in the model.
    while (!lastError().isValid() && canFetchMore()) {
        fetchMore();
    }
    if (lastError().isValid()) {
        error = lastError().text();
        return false;
    }
    return true;
}
