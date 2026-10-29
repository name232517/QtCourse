#include "mainwindow.h"
#include "./ui_mainwindow.h"

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
