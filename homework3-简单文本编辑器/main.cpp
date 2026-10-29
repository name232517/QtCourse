#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

// 程序入口：设置程序名与窗口图标，显示主窗口后进入事件循环
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("编辑器"));
    app.setApplicationDisplayName(QStringLiteral("编辑器"));
    app.setWindowIcon(QIcon(QStringLiteral(":/images/notebook-outline.svg")));

    MainWindow window;
    window.show();

    return app.exec();
}
