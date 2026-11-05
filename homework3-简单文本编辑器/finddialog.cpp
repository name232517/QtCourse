#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
{
    ui->setupUi(this);

    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &FindDialog::onFindClicked);

    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &FindDialog::onFindClicked);
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
    ui->lineEditFind->selectAll();
    ui->lineEditFind->setFocus();
}

QString FindDialog::findText() const
{
    return ui->lineEditFind->text();
}

bool FindDialog::caseSensitive() const
{
    return ui->checkCase->isChecked();
}

bool FindDialog::searchBackward() const
{
    return ui->radioUp->isChecked();
}

void FindDialog::onFindClicked()
{
    emit findRequested(findText(), caseSensitive(), searchBackward());
}
