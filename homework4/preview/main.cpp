#include "previewwindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("诊疗工作台"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QFont font = app.font();
#ifdef Q_OS_WIN
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
#endif
    font.setPixelSize(14);
    app.setFont(font);

    QFile theme(QStringLiteral(":/preview/theme.qss"));
    if (theme.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    }

    PreviewWindow window;
    window.show();
    return app.exec();
}
