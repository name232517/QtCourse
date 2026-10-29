#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "aboutdialog.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QLabel>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 状态栏信息区：length / lines / Ln / Col / 姓名
// 标签先在设计器里建好（设置文字与对齐），运行时再挂成 permanent 部件，
// 这样它们会一直贴在状态栏右端，窗口缩放时也不会被临时提示信息挤掉。
void MainWindow::initStatusBar()
{
    const QList<QLabel *> labels = {ui->labelLength, ui->labelLines, ui->labelLn,
                                    ui->labelCol, ui->labelName};
    for (QLabel *label : labels) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumWidth(72);
        statusBar()->addPermanentWidget(label);
    }
}

// 查找对话框：非模态，可以一边开着一边看编辑区
void MainWindow::on_actionFind_triggered()
{
    if (m_findDialog == nullptr)
        m_findDialog = new FindDialog(this);

    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

// 替换对话框：非模态，同上
void MainWindow::on_actionReplace_triggered()
{
    if (m_replaceDialog == nullptr)
        m_replaceDialog = new ReplaceDialog(this);

    m_replaceDialog->show();
    m_replaceDialog->raise();
    m_replaceDialog->activateWindow();
}

// 关于对话框：只是看一眼，用模态，点 OK 或关闭按钮都返回
void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}
