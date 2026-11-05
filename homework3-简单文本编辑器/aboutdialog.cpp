#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QIcon>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->labelLogo->setPixmap(QIcon(QStringLiteral(":/images/notebook-outline.svg")).pixmap(96, 96));

    connect(ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
