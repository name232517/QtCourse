#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QIcon>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // .ui 里直接放 SVG 时，Qt 会按图标的默认尺寸（24px）生成位图，拉大到 96px 会发糊；
    // 改用 QIcon 让 SVG 按 96px 重新矢量渲染，边缘才清晰
    ui->labelLogo->setPixmap(QIcon(QStringLiteral(":/images/notebook-outline.svg")).pixmap(96, 96));

    connect(ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
