#pragma once

#include <QSqlQueryModel>

class PatientTableModel final : public QSqlQueryModel
{
public:
    enum Column { Id, Name, Sex, BirthDate, IdentityCard, Mobile, Height, Weight };

    bool load(const QSqlDatabase &database, QString &error);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};
