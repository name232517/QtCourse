#pragma once

#include "patientrecord.h"
#include "staffrecord.h"

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>

class Database final
{
public:
    enum class CredentialLookup { Found, UnknownUser, Error };
    enum class PatientRemoval { Removed, Missing, Error };
    enum class PatientSave { Saved, Stale, Error };
    enum class DepartmentSave { Saved, Stale, Duplicate, Error };
    enum class DepartmentRemoval { Removed, Missing, InUse, Error };
    enum class DoctorSave { Saved, Stale, Duplicate, MissingDepartment, Error };
    enum class DoctorRemoval { Removed, Missing, Error };

    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open(const QString &filePath, QString &error);
    bool addDemoData(QString &error);
    // userId may be null; when given it receives the primary key of the matched account.
    CredentialLookup passwordDigestForUser(const QString &username, QByteArray &digest,
                                           QString *userId = nullptr) const;
    PatientRemoval removePatient(const QString &id, QString &error) const;
    PatientSave savePatient(PatientRecord &patient, const std::optional<PatientRecord> &original,
                            QString &error) const;
    DepartmentSave saveDepartment(DepartmentRecord &department,
                                  const std::optional<DepartmentRecord> &original, QString &error) const;
    DepartmentRemoval removeDepartment(const QString &id, QString &error) const;
    DoctorSave saveDoctor(DoctorRecord &doctor, const std::optional<DoctorRecord> &original,
                          QString &error) const;
    DoctorRemoval removeDoctor(const QString &id, QString &error) const;
    bool appendHistory(const QString &userId, const QString &event, QString &error) const;

    // Destroy queries and models that use this connection before destroying Database.
    QSqlDatabase connection() const;

private:
    bool initializeSchema(QString &error);

    QString connectionName_;
    QSqlDatabase db_;
};
