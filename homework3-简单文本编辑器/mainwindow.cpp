#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "aboutdialog.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextDocument>
#include <QTextStream>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

// 统一用 UTF-8 读写文本：Qt 6 用 setEncoding，Qt 5 用 setCodec
void setUtf8Encoding(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

const char *const kFileFilter = "文本文件 (*.txt);;所有文件 (*.*)";

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initStatusBar();

    // 编辑区内容一改，标题上的 * 跟着变
    connect(ui->textEdit->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);

    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    setCurrentFile(QString());
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

// 标题格式：*文件名 - 编辑器（有未保存修改时前面加 *）
void MainWindow::updateWindowTitle()
{
    const QString name = m_filePath.isEmpty() ? tr("新建文本文件")
                                              : QFileInfo(m_filePath).fileName();
    const QString mark = ui->textEdit->document()->isModified() ? QStringLiteral("*")
                                                                : QString();
    setWindowTitle(QStringLiteral("%1%2 - %3").arg(mark, name, tr("编辑器")));
}

// 记下当前文件：路径为空表示新建的、还没保存过
void MainWindow::setCurrentFile(const QString &path)
{
    m_filePath = path;
    ui->textEdit->document()->setModified(false);
    updateWindowTitle();
}

// ---------------------------------------------------------------------------
// 文件菜单：新建 / 打开 / 保存 / 另存为 / 退出
// ---------------------------------------------------------------------------

void MainWindow::on_actionNew_triggered()
{
    if (!maybeSave())   // 当前文档有改动时，先问要不要保存
        return;

    ui->textEdit->clear();
    setCurrentFile(QString());
    statusBar()->showMessage(tr("已新建文档"), 2000);
}

void MainWindow::on_actionOpen_triggered()
{
    if (!maybeSave())
        return;

    const QString path = QFileDialog::getOpenFileName(this, tr("打开文件"), m_lastDir,
                                                      tr(kFileFilter));
    if (path.isEmpty())   // 用户点了取消
        return;

    loadFile(path);
}

void MainWindow::on_actionSave_triggered()
{
    saveFile();
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveFileAs();
}

void MainWindow::on_actionExit_triggered()
{
    close();   // 走 closeEvent，同样会提示保存
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

bool MainWindow::maybeSave()
{
    if (!ui->textEdit->document()->isModified())
        return true;

    const QString name = m_filePath.isEmpty() ? tr("新建文本文件")
                                              : QFileInfo(m_filePath).fileName();
    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("编辑器"),
        tr("文档「%1」已修改，是否保存？").arg(name),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    switch (ret) {
    case QMessageBox::Save:
        return saveFile();
    case QMessageBox::Discard:
        return true;
    default:
        return false;
    }
}

bool MainWindow::saveFile()
{
    if (m_filePath.isEmpty())   // 新建的文档，先问路径
        return saveFileAs();

    return writeFile(m_filePath);
}

bool MainWindow::saveFileAs()
{
    QString path = m_filePath;
    if (path.isEmpty())
        path = QDir(m_lastDir).filePath(QStringLiteral("新建文本文件.txt"));

    path = QFileDialog::getSaveFileName(this, tr("另存为"), path, tr(kFileFilter));
    if (path.isEmpty())   // 用户点了取消
        return false;

    return writeFile(path);
}

bool MainWindow::writeFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(this, tr("保存失败"),
                              tr("无法写入文件：\n%1\n\n原因：%2")
                                  .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    QTextStream out(&file);
    setUtf8Encoding(out);
    out << ui->textEdit->toPlainText();
    out.flush();

    const bool ok = (out.status() == QTextStream::Ok);
    const QString error = file.errorString();
    file.close();

    if (!ok) {
        QMessageBox::critical(this, tr("保存失败"),
                              tr("写入文件时出错：\n%1\n\n原因：%2")
                                  .arg(QDir::toNativeSeparators(path), error));
        return false;
    }

    m_lastDir = QFileInfo(path).absolutePath();
    setCurrentFile(path);
    statusBar()->showMessage(tr("已保存到 %1").arg(QDir::toNativeSeparators(path)), 3000);
    return true;
}

bool MainWindow::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开失败"),
                             tr("无法打开文件：\n%1\n\n原因：%2")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    QTextStream in(&file);
    setUtf8Encoding(in);
    const QString text = in.readAll();
    const bool ok = (in.status() == QTextStream::Ok);
    file.close();

    if (!ok) {
        QMessageBox::warning(this, tr("打开失败"),
                             tr("读取文件时出错：\n%1").arg(QDir::toNativeSeparators(path)));
        return false;
    }

    ui->textEdit->setPlainText(text);
    m_lastDir = QFileInfo(path).absolutePath();
    setCurrentFile(path);
    statusBar()->showMessage(tr("已打开 %1").arg(QDir::toNativeSeparators(path)), 3000);
    return true;
}

// ---------------------------------------------------------------------------
// 对话框
// ---------------------------------------------------------------------------

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
