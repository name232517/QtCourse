#include "database.h"
#include "patienttablemodel.h"
#include "previewwindow.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTranslator>

namespace {
int reportDatabaseError(const QString &title, const QString &error)
{
    QMessageBox dialog(QMessageBox::Critical, title, {}, QMessageBox::Close);
    dialog.setTextFormat(Qt::PlainText);
    dialog.setText(title + QStringLiteral("\n\n") + error);
    dialog.button(QMessageBox::Close)->setText(QStringLiteral("关闭"));
    dialog.exec();
    return 1;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QT3Ui"));
    app.setApplicationDisplayName(QStringLiteral("诊疗工作台"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Qt 自带的对话框按钮（QInputDialog / QDialogButtonBox）默认是英文，加载中文翻译。
    QTranslator translator;
    if (translator.load(QLocale(QLocale::Chinese, QLocale::China), QStringLiteral("qtbase"),
                        QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&translator);
    }

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

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("诊疗工作台"));
    parser.addHelpOption();
    const QCommandLineOption databaseOption(QStringLiteral("database"),
        QStringLiteral("指定 SQLite 数据库文件。"), QStringLiteral("文件"));
    const QCommandLineOption demoOption(QStringLiteral("demo-data"),
        QStringLiteral("向所用数据库添加实验演示数据。"));
    parser.addOption(databaseOption);
    parser.addOption(demoOption);
    parser.process(app);
    if (!parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
    }

    const QString databaseError = QStringLiteral("数据库连接失败");
    QString filePath;
    if (parser.isSet(databaseOption)) {
        const QString path = parser.value(databaseOption);
        if (!path.trimmed().isEmpty()) {
            filePath = QFileInfo(path).absoluteFilePath();
        }
    } else {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (directory.isEmpty() || !QDir().mkpath(directory)) {
            return reportDatabaseError(databaseError,
                QStringLiteral("无法创建应用的数据库目录。%1").arg(QDir::toNativeSeparators(directory)));
        }
        filePath = QDir(directory).filePath(QStringLiteral("hospital.sqlite"));
    }

    Database database;
    QString error;
    if (!database.open(filePath, error)) {
        return reportDatabaseError(databaseError, error);
    }
    if (parser.isSet(demoOption) && !database.addDemoData(error)) {
        return reportDatabaseError(QStringLiteral("演示数据导入失败"), error);
    }

    PatientTableModel patients(database);
    if (!patients.load(error)) {
        return reportDatabaseError(QStringLiteral("数据库读取失败"), error);
    }

    PreviewWindow window(database, patients);
    window.show();
    return app.exec();
}
