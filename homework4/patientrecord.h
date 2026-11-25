#pragma once

#include <QString>

#include <optional>

struct PatientRecord
{
    QString id;
    QString name;
    QString identityCard;
    int sex = -1;
    QString birthDate;
    QString mobile;
    std::optional<double> height;
    std::optional<double> weight;
    QString createdTimestamp;

    static QString sexLabel(int value)
    {
        switch (value) {
        case 1:
            return QStringLiteral("男");
        case 2:
            return QStringLiteral("女");
        default:
            return QStringLiteral("未知");
        }
    }
};
