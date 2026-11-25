#pragma once

#include <QSqlDatabase>
#include <QString>

class Database final
{
public:
    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open(const QString &filePath, QString &error);
    bool addDemoData(QString &error);

    // Destroy queries and models that use this connection before destroying Database.
    QSqlDatabase connection() const;

private:
    bool initializeSchema(QString &error);

    QString connectionName_;
    QSqlDatabase db_;
};
