#include "replacedialog.h"
#include "ui_replacedialog.h"

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);

    // 非模态窗口，取消按钮只关闭自己
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &ReplaceDialog::onFindClicked);
    connect(ui->btnReplace, &QPushButton::clicked, this, &ReplaceDialog::onReplaceClicked);
    connect(ui->btnReplaceAll, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAllClicked);
    // 在「查找目标」里按回车 = 查找下一个
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &ReplaceDialog::onFindClicked);
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

void ReplaceDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
    ui->lineEditFind->selectAll();
    ui->lineEditFind->setFocus();
}

QString ReplaceDialog::findText() const
{
    return ui->lineEditFind->text();
}

QString ReplaceDialog::replaceText() const
{
    return ui->lineEditReplace->text();
}

bool ReplaceDialog::caseSensitive() const
{
    return ui->checkCase->isChecked();
}

bool ReplaceDialog::searchBackward() const
{
    return ui->radioUp->isChecked();
}

void ReplaceDialog::onFindClicked()
{
    emit findRequested(findText(), caseSensitive(), searchBackward());
}

void ReplaceDialog::onReplaceClicked()
{
    emit replaceRequested(findText(), replaceText(), caseSensitive(), searchBackward());
}

void ReplaceDialog::onReplaceAllClicked()
{
    emit replaceAllRequested(findText(), replaceText(), caseSensitive(), searchBackward());
}
