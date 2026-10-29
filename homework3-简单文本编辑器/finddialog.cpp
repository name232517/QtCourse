#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
{
    ui->setupUi(this);

    // 非模态窗口，取消按钮只关闭自己
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

FindDialog::~FindDialog()
{
    delete ui;
}
