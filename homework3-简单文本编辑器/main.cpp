#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

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
