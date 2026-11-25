#pragma once

#include "patientrecord.h"

#include <QSqlQueryModel>

class Database;

class PatientTableModel final : public QSqlQueryModel
{
    Q_OBJECT

public:
    enum Column { Id, Name, Sex, BirthDate, IdentityCard, Mobile, Height, Weight, CreatedTimestamp };

    explicit PatientTableModel(const Database &database, QObject *parent = nullptr);
    bool load(QString &error);
    PatientRecord patientAt(int row) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
    void patientSaved(const QString &id);
    void patientSaveFailed(const QString &error);

private:
    const Database &database_;
};
