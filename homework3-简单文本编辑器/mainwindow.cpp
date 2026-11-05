#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QCloseEvent>
#include <QClipboard>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

void setUtf8Encoding(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

const char *const kFileFilter = "文本文件 (*.txt);;所有文件 (*.*)";

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initStatusBar();
    initEditActions();
    initViewActions();

    on_actionLineNumbers_toggled(ui->actionLineNumbers->isChecked());

    initAutoSave();

    connect(ui->textEdit->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);

    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    setCurrentFile(QString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initStatusBar()
{
    const QList<QLabel *> labels = {ui->labelLength, ui->labelLines, ui->labelLn,
                                    ui->labelCol, ui->labelName};
    for (QLabel *label : labels) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumWidth(72);
        statusBar()->addPermanentWidget(label);
    }

    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusInfo);
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusInfo);
    connect(ui->textEdit, &QPlainTextEdit::selectionChanged, this, &MainWindow::updateStatusInfo);

    updateStatusInfo();
}

void MainWindow::updateStatusInfo()
{
    const QTextCursor cursor = ui->textEdit->textCursor();

    ui->labelLength->setText(tr("length: %1").arg(ui->textEdit->toPlainText().length()));
    ui->labelLines->setText(tr("lines: %1").arg(ui->textEdit->blockCount()));
    ui->labelLn->setText(tr("Ln: %1").arg(cursor.blockNumber() + 1));
    ui->labelCol->setText(tr("Col: %1").arg(cursor.positionInBlock() + 1));
}

void MainWindow::initEditActions()
{
    connect(ui->actionUndo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->textEdit, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->textEdit, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->textEdit, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);

    QTextDocument *doc = ui->textEdit->document();
    connect(doc, &QTextDocument::undoAvailable, this, &MainWindow::updateEditActions);
    connect(doc, &QTextDocument::redoAvailable, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::selectionChanged, this, &MainWindow::updateEditActions);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateEditActions);

    updateEditActions();
}

void MainWindow::updateEditActions()
{
    const QTextDocument *doc = ui->textEdit->document();
    const bool hasSelection = ui->textEdit->textCursor().hasSelection();

    ui->actionUndo->setEnabled(doc->isUndoAvailable());
    ui->actionRedo->setEnabled(doc->isRedoAvailable());
    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(ui->textEdit->canPaste());
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_filePath.isEmpty() ? tr("新建文本文件")
                                              : QFileInfo(m_filePath).fileName();
    const QString mark = ui->textEdit->document()->isModified() ? QStringLiteral("*")
                                                                : QString();
    setWindowTitle(QStringLiteral("%1%2 - %3").arg(mark, name, tr("编辑器")));
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_filePath = path;
    ui->textEdit->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::on_actionNew_triggered()
{
    if (!maybeSave())
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
    if (path.isEmpty())
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
    close();
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
    if (m_filePath.isEmpty())
        return saveFileAs();

    return writeFile(m_filePath);
}

bool MainWindow::saveFileAs()
{
    QString path = m_filePath;
    if (path.isEmpty())
        path = QDir(m_lastDir).filePath(QStringLiteral("新建文本文件.txt"));

    path = QFileDialog::getSaveFileName(this, tr("另存为"), path, tr(kFileFilter));
    if (path.isEmpty())
        return false;

    return writeFile(path);
}

bool MainWindow::writeFile(const QString &path, bool autoSaved)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        const QString reason = file.errorString();
        if (autoSaved) {
            statusBar()->showMessage(tr("自动保存失败：%1").arg(reason), 5000);
        } else {
            QMessageBox::critical(this, tr("保存失败"),
                                  tr("无法写入文件：\n%1\n\n原因：%2")
                                      .arg(QDir::toNativeSeparators(path), reason));
        }
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
        if (autoSaved) {
            statusBar()->showMessage(tr("自动保存失败：%1").arg(error), 5000);
        } else {
            QMessageBox::critical(this, tr("保存失败"),
                                  tr("写入文件时出错：\n%1\n\n原因：%2")
                                      .arg(QDir::toNativeSeparators(path), error));
        }
        return false;
    }

    m_lastDir = QFileInfo(path).absolutePath();
    setCurrentFile(path);
    statusBar()->showMessage(autoSaved ? tr("已自动保存到 %1").arg(QDir::toNativeSeparators(path))
                                       : tr("已保存到 %1").arg(QDir::toNativeSeparators(path)),
                             3000);
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

void MainWindow::on_actionFind_triggered()
{
    if (m_findDialog == nullptr) {
        m_findDialog = new FindDialog(this);
        connect(m_findDialog, &FindDialog::findRequested, this,
                [this](const QString &text, bool caseSensitive, bool backward) {
                    findNext(text, caseSensitive, backward);
                    bringToFront(m_findDialog);
                });
    }

    const QString selected = ui->textEdit->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        m_findDialog->setFindText(selected);

    bringToFront(m_findDialog);
}

void MainWindow::on_actionReplace_triggered()
{
    if (m_replaceDialog == nullptr) {
        m_replaceDialog = new ReplaceDialog(this);

        connect(m_replaceDialog, &ReplaceDialog::findRequested,
                this, &MainWindow::onReplaceFindRequested);
        connect(m_replaceDialog, &ReplaceDialog::replaceRequested, this,
                [this](const QString &text, const QString &replacement, bool caseSensitive,
                       bool backward) {
                    replaceOne(text, replacement, caseSensitive, backward);
                    bringToFront(m_replaceDialog);
                });
        connect(m_replaceDialog, &ReplaceDialog::replaceAllRequested, this,
                [this](const QString &text, const QString &replacement, bool caseSensitive,
                       bool backward) {
                    replaceAll(text, replacement, caseSensitive, backward);
                    bringToFront(m_replaceDialog);
                });
    }

    const QString selected = ui->textEdit->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        m_replaceDialog->setFindText(selected);

    bringToFront(m_replaceDialog);
}

void MainWindow::bringToFront(QWidget *dialog)
{
    if (dialog == nullptr)
        return;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

QTextCursor MainWindow::findMatch(const QString &text, bool caseSensitive, bool backward,
                                  bool *wrapped) const
{
    if (wrapped != nullptr)
        *wrapped = false;

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;

    QTextDocument *doc = ui->textEdit->document();

    QTextCursor found = doc->find(text, ui->textEdit->textCursor(), flags);
    if (!found.isNull())
        return found;

    QTextCursor from(doc);
    from.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    found = doc->find(text, from, flags);
    if (!found.isNull() && wrapped != nullptr)
        *wrapped = true;
    return found;
}

void MainWindow::findNext(const QString &text, bool caseSensitive, bool backward)
{
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("查找"), tr("请先输入要查找的内容。"));
        return;
    }

    bool wrapped = false;
    const QTextCursor found = findMatch(text, caseSensitive, backward, &wrapped);

    if (found.isNull()) {
        QMessageBox::information(this, tr("查找"), tr("找不到「%1」。").arg(text));
        statusBar()->showMessage(tr("未找到：%1").arg(text), 3000);
        return;
    }

    ui->textEdit->setTextCursor(found);
    ui->textEdit->ensureCursorVisible();
    statusBar()->showMessage(wrapped ? tr("已绕回另一头，找到：%1").arg(text)
                                     : tr("找到：%1").arg(text),
                             3000);
}

void MainWindow::onReplaceFindRequested(const QString &text, bool caseSensitive, bool backward)
{
    findNext(text, caseSensitive, backward);
    bringToFront(m_replaceDialog);
}

void MainWindow::replaceOne(const QString &text, const QString &replacement, bool caseSensitive,
                            bool backward)
{
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("请先输入要查找的内容。"));
        return;
    }

    QTextCursor cursor = ui->textEdit->textCursor();
    bool replaced = false;

    if (cursor.hasSelection()) {
        const QString selected = cursor.selectedText();
        const bool same = caseSensitive ? (selected == text)
                                        : (selected.compare(text, Qt::CaseInsensitive) == 0);
        if (same) {
            cursor.insertText(replacement);
            replaced = true;
        }
    }

    if (!replaced) {
        bool wrapped = false;
        const QTextCursor next = findMatch(text, caseSensitive, backward, &wrapped);
        if (next.isNull()) {
            QMessageBox::information(this, tr("替换"), tr("找不到「%1」。").arg(text));
            statusBar()->showMessage(tr("未找到：%1").arg(text), 3000);
            return;
        }
        ui->textEdit->setTextCursor(next);
        ui->textEdit->ensureCursorVisible();
        statusBar()->showMessage(tr("已找到「%1」，再点一次「替换」即可").arg(text), 3000);
        return;
    }

    bool wrapped = false;
    const QTextCursor next = findMatch(text, caseSensitive, backward, &wrapped);
    if (next.isNull()) {
        statusBar()->showMessage(tr("已替换 1 处，后面没有更多匹配了"), 3000);
        return;
    }
    ui->textEdit->setTextCursor(next);
    ui->textEdit->ensureCursorVisible();
    statusBar()->showMessage(wrapped ? tr("已替换 1 处，并绕回另一头找到下一个")
                                     : tr("已替换 1 处"),
                             3000);
}

void MainWindow::replaceAll(const QString &text, const QString &replacement, bool caseSensitive,
                            bool backward)
{
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("请先输入要查找的内容。"));
        return;
    }

    QTextDocument *doc = ui->textEdit->document();

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;

    QTextCursor start(doc);
    start.movePosition(backward ? QTextCursor::End : QTextCursor::Start);

    int count = 0;
    QTextCursor found = doc->find(text, start, flags);
    if (!found.isNull()) {

        QTextCursor blockCursor(doc);
        blockCursor.beginEditBlock();
        while (!found.isNull()) {
            const int replaceStart = found.selectionStart();
            found.insertText(replacement);
            ++count;

            QTextCursor from(doc);
            from.setPosition(backward ? replaceStart : found.position());
            found = doc->find(text, from, flags);
        }
        blockCursor.endEditBlock();
    }

    if (count == 0) {
        QMessageBox::information(this, tr("替换"), tr("找不到「%1」。").arg(text));
        statusBar()->showMessage(tr("未找到：%1").arg(text), 3000);
        return;
    }

    statusBar()->showMessage(tr("共替换 %1 处").arg(count), 3000);
    QMessageBox::information(this, tr("替换"), tr("共替换 %1 处。").arg(count));
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionFont_triggered()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, ui->textEdit->font(), this, tr("字体"));
    if (!ok)
        return;

    ui->textEdit->setFont(font);
    ui->textEdit->refreshLineNumberArea();

    statusBar()->showMessage(tr("当前字体：%1 %2pt").arg(font.family()).arg(font.pointSize()), 3000);
}

void MainWindow::on_actionFontColor_triggered()
{

    QColor current = ui->textEdit->textCursor().charFormat().foreground().color();
    if (!current.isValid())
        current = ui->textEdit->palette().color(QPalette::Text);

    const QColor color = QColorDialog::getColor(current, this, tr("字体颜色"));
    if (!color.isValid())
        return;

    applyTextColor(color);
}

void MainWindow::on_actionFontBackground_triggered()
{
    const QColor current = ui->textEdit->textCursor().charFormat().background().color();
    const QColor color = QColorDialog::getColor(current, this, tr("字体背景色"));
    if (!color.isValid())
        return;

    applyTextBackground(color);
}

void MainWindow::on_actionEditorBackground_triggered()
{
    const QColor current = ui->textEdit->palette().color(QPalette::Base);
    const QColor color = QColorDialog::getColor(current, this, tr("编辑器背景色"));
    if (!color.isValid())
        return;

    applyEditorBackground(color);
}

void MainWindow::applyTextColor(const QColor &color)
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.mergeCharFormat(format);
    ui->textEdit->mergeCurrentCharFormat(format);
}

void MainWindow::applyTextBackground(const QColor &color)
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    format.setBackground(color);
    cursor.mergeCharFormat(format);
    ui->textEdit->mergeCurrentCharFormat(format);
}

void MainWindow::applyEditorBackground(const QColor &color)
{
    QPalette palette = ui->textEdit->palette();
    palette.setColor(QPalette::Base, color);
    ui->textEdit->setPalette(palette);
}

void MainWindow::on_actionWrap_toggled(bool checked)
{
    ui->textEdit->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void MainWindow::on_actionLineNumbers_toggled(bool checked)
{
    if (ui->textEdit->isLineNumbersVisible() == checked)
        return;

    ui->textEdit->setLineNumbersVisible(checked);
    ui->textEdit->ensureCursorVisible();
    statusBar()->showMessage(checked ? tr("已显示行号") : tr("已隐藏行号"), 2000);
}

namespace {

const int kAutoSaveIntervalMs = 30 * 1000;
}

void MainWindow::initAutoSave()
{
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(kAutoSaveIntervalMs);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    m_autoSaveTimer->start();
}

void MainWindow::autoSave()
{
    if (m_filePath.isEmpty())
        return;

    if (!ui->textEdit->document()->isModified())
        return;

    writeFile(m_filePath, true);
}

void MainWindow::initViewActions()
{
    ui->mainToolBar->setVisible(ui->actionToolBar->isChecked());
    statusBar()->setVisible(ui->actionStatusBar->isChecked());

    connect(ui->mainToolBar, &QToolBar::visibilityChanged, this, [this](bool visible) {
        if (ui->actionToolBar->isChecked() == visible)
            return;

        QSignalBlocker blocker(ui->actionToolBar);
        ui->actionToolBar->setChecked(visible);
    });
}

void MainWindow::on_actionToolBar_toggled(bool checked)
{
    ui->mainToolBar->setVisible(checked);
}

void MainWindow::on_actionStatusBar_toggled(bool checked)
{
    statusBar()->setVisible(checked);
}
