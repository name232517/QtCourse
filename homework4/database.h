#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>

class Database final
{
public:
    enum class CredentialLookup { Found, UnknownUser, Error };
    enum class PatientRemoval { Removed, Missing, Error };

    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open(const QString &filePath, QString &error);
    bool addDemoData(QString &error);
    CredentialLookup passwordDigestForUser(const QString &username, QByteArray &digest) const;
    PatientRemoval removePatient(const QString &id, QString &error) const;

    // Destroy queries and models that use this connection before destroying Database.
    QSqlDatabase connection() const;

private:
    bool initializeSchema(QString &error);

    QString connectionName_;
    QSqlDatabase db_;
};
