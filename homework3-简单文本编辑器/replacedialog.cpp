#include "replacedialog.h"
#include "ui_replacedialog.h"

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);

    // 非模态窗口，取消按钮只关闭自己
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}
