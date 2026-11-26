#pragma once

#include <QString>

struct DepartmentRecord
{
    QString id;
    QString name;
};

struct DoctorRecord
{
    QString id;
    QString employeeNo;
    QString name;
    QString departmentId; // 空字符串表示尚未分配科室
};
