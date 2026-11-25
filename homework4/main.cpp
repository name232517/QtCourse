#include "database.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("QT3"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("医院诊疗系统数据库初始化工具"));
    parser.addHelpOption();
    const QCommandLineOption databaseOption(QStringLiteral("database"),
        QStringLiteral("指定 SQLite 数据库文件路径。"), QStringLiteral("文件"),
        QStringLiteral("hospital.sqlite"));
    const QCommandLineOption demoOption(QStringLiteral("demo-data"),
        QStringLiteral("添加一条演示账号和三条虚构的患者记录。"));
    parser.addOption(databaseOption);
    parser.addOption(demoOption);
    parser.process(app);
    if (!parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
    }

    const QString pathOption = parser.value(databaseOption);
    const QString filePath = pathOption.trimmed().isEmpty()
        ? QString()
        : QFileInfo(pathOption).absoluteFilePath();
    Database database;
    QString error;
    if (!database.open(filePath, error)
        || (parser.isSet(demoOption) && !database.addDemoData(error))) {
        QTextStream(stderr) << error << Qt::endl;
        return 1;
    }

    QTextStream(stdout) << QStringLiteral("数据库已就绪。")
                        << QDir::toNativeSeparators(filePath) << Qt::endl;
    return 0;
}
