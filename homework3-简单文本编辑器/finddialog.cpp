#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
{
    ui->setupUi(this);

    // 非模态窗口，取消按钮只关闭自己
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    // 在输入框里按回车，效果和点「查找下一个」一样
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

// 把当前界面上的三个条件打包成信号发出去
void FindDialog::onFindClicked()
{
    emit findRequested(findText(), caseSensitive(), searchBackward());
}
