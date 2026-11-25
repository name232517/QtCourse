#include "patientsexdelegate.h"

#include "patientrecord.h"

#include <QComboBox>

QWidget *PatientSexDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                         const QModelIndex &) const
{
    auto *combo = new QComboBox(parent);
    combo->setObjectName(QStringLiteral("patientSexEditor"));
    combo->setAccessibleName(QStringLiteral("患者性别"));
    for (int value = 0; value <= 2; ++value) {
        combo->addItem(PatientRecord::sexLabel(value), value);
    }
    auto *delegate = const_cast<PatientSexDelegate *>(this);
    connect(combo, &QComboBox::activated, delegate, [delegate, combo] {
        emit delegate->commitData(combo);
        emit delegate->closeEditor(combo);
    });
    return combo;
}

void PatientSexDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *combo = static_cast<QComboBox *>(editor);
    combo->setCurrentIndex(combo->findData(index.data(Qt::EditRole)));
}

void PatientSexDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    auto *combo = static_cast<QComboBox *>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}
