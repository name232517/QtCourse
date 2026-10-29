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
#include <QToolBar>

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
    initEditActions();
    initViewActions();

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

    // 状态栏里的数字跟着编辑区变化：改内容、移动光标都要刷新
    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusInfo);
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusInfo);
    connect(ui->textEdit, &QPlainTextEdit::selectionChanged, this, &MainWindow::updateStatusInfo);

    updateStatusInfo();
}

// 刷新状态栏：总字符数、总行数、光标所在行与列（都从 1 开始数）
void MainWindow::updateStatusInfo()
{
    const QTextCursor cursor = ui->textEdit->textCursor();

    ui->labelLength->setText(tr("length: %1").arg(ui->textEdit->toPlainText().length()));
    ui->labelLines->setText(tr("lines: %1").arg(ui->textEdit->blockCount()));
    ui->labelLn->setText(tr("Ln: %1").arg(cursor.blockNumber() + 1));
    ui->labelCol->setText(tr("Col: %1").arg(cursor.positionInBlock() + 1));
}

// ---------------------------------------------------------------------------
// 编辑菜单：撤销 / 恢复 / 剪切 / 复制 / 粘贴 / 全选
// ---------------------------------------------------------------------------

// 把 Action 接到编辑区上同名操作，并让它们的可用状态跟着编辑区实时变化：
// 没有可撤销的步骤、没有选中文字、剪贴板里没有文本时，对应菜单项和工具栏按钮变灰。
void MainWindow::initEditActions()
{
    connect(ui->actionUndo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->textEdit, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->textEdit, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->textEdit, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);

    // 这些信号覆盖了「改内容 / 撤销重做 / 选中变化 / 剪贴板变化」四种情况
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
    ui->actionCut->setEnabled(hasSelection);    // 没选中东西时剪切/复制无意义
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(ui->textEdit->canPaste());
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
    if (m_findDialog == nullptr) {
        m_findDialog = new FindDialog(this);
        connect(m_findDialog, &FindDialog::findRequested, this,
                [this](const QString &text, bool caseSensitive, bool backward) {
                    findNext(text, caseSensitive, backward);
                    bringToFront(m_findDialog);
                });
    }

    // 编辑区里正好选中了一段文字，就把它填进「查找目标」，省得再敲一遍
    const QString selected = ui->textEdit->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        m_findDialog->setFindText(selected);

    bringToFront(m_findDialog);
}

// 替换对话框：非模态，同样可以一边开着一边看编辑区
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

// 把非模态对话框显示出来并提到最前，方便接着操作
void MainWindow::bringToFront(QWidget *dialog)
{
    if (dialog == nullptr)
        return;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

// 查找的核心：先按方向从光标处找，找不到再绕回文档另一头找一次。
// 找不到返回空光标；wrapped 用来告诉调用方「这次是绕回来的」。
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

    // 第一轮：从光标处开始找。光标带着上一次的匹配选区时，
    // QTextDocument::find 会从选区末尾（向上查找时是选区开头）接着搜，正好是「下一个」。
    QTextCursor found = doc->find(text, ui->textEdit->textCursor(), flags);
    if (!found.isNull())
        return found;

    // 第二轮：已经到文档尽头了，绕回另一头再找一次
    QTextCursor from(doc);
    from.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    found = doc->find(text, from, flags);
    if (!found.isNull() && wrapped != nullptr)
        *wrapped = true;
    return found;
}

// 在编辑区里查找下一个匹配：
// 找到就选中它并滚到可见处，光标带着选区，下次再点就从这个匹配后面继续；
// 两头都没有才提示「找不到」，此时光标原地不动。
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

// 替换对话框里点「查找下一个」：和普通查找完全一样，只是查完把焦点还给它自己
void MainWindow::onReplaceFindRequested(const QString &text, bool caseSensitive, bool backward)
{
    findNext(text, caseSensitive, backward);
    bringToFront(m_replaceDialog);
}

// 替换一处：
// 当前选中的文字正好是查找词，就直接换掉，再把下一个匹配选中，方便连着点；
// 没选中（或选中的不是查找词）就先把下一个匹配找出来，等用户确认后再换。
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

    // 替换成功：顺手把下一个匹配选中
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

// 全部替换：整篇扫一遍，一个都不放过的同时合并成「一步」修改，
// 这样按一次 Ctrl+Z 就能整篇回到替换前。
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

    // 不管光标在哪，都从整篇的开头（向上替换时从末尾）开始
    QTextCursor start(doc);
    start.movePosition(backward ? QTextCursor::End : QTextCursor::Start);

    int count = 0;
    QTextCursor found = doc->find(text, start, flags);
    if (!found.isNull()) {
        // beginEditBlock / endEditBlock 之间的修改在撤销栈里合成一步
        QTextCursor blockCursor(doc);
        blockCursor.beginEditBlock();
        while (!found.isNull()) {
            const int replaceStart = found.selectionStart();
            found.insertText(replacement);
            ++count;

            // 向下替换从替换后的位置接着找；向上替换回到替换处再往前找。
            // 这样即使替换词里又含有查找词，也不会在原处打转。
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

// 关于对话框：只是看一眼，用模态，点 OK 或关闭按钮都返回
void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

// ---------------------------------------------------------------------------
// 格式菜单：字体 / 字体颜色 / 字体背景色 / 编辑器背景色 / 自动换行 / 显示行号
// ---------------------------------------------------------------------------

void MainWindow::on_actionFont_triggered()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, ui->textEdit->font(), this, tr("字体"));
    if (!ok)   // 用户点了取消
        return;

    ui->textEdit->setFont(font);
    ui->textEdit->refreshLineNumberArea();   // 字号变了，行号栏宽度要重新算

    statusBar()->showMessage(tr("当前字体：%1 %2pt").arg(font.family()).arg(font.pointSize()), 3000);
}

void MainWindow::on_actionFontColor_triggered()
{
    // QPlainTextEdit 没有 textColor()，当前颜色从光标处的字符格式里取；没设过就用调色板的文字色
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

// 改文字颜色：有选中就只改选中的，没选中就整篇一起改
void MainWindow::applyTextColor(const QColor &color)
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.mergeCharFormat(format);
    ui->textEdit->mergeCurrentCharFormat(format);   // 之后新输入的字也用这个颜色
}

// 改文字背景色：规则同字体颜色
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

// 编辑器背景色单独走调色板，不影响文字本身的背景
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

// 行号栏由 CodeEditor 自己维护，这里只转告开关
void MainWindow::on_actionLineNumbers_toggled(bool checked)
{
    ui->textEdit->setLineNumbersVisible(checked);
}

// ---------------------------------------------------------------------------
// 查看菜单：工具栏、状态栏的显示与隐藏
// ---------------------------------------------------------------------------

// 打开程序时先让勾选状态和实际显示一致，再把两边的变化串起来：
// 工具栏还能被右键菜单或拖动关掉，那时菜单里的勾也要取消
void MainWindow::initViewActions()
{
    ui->mainToolBar->setVisible(ui->actionToolBar->isChecked());
    statusBar()->setVisible(ui->actionStatusBar->isChecked());

    connect(ui->mainToolBar, &QToolBar::visibilityChanged, this, [this](bool visible) {
        if (ui->actionToolBar->isChecked() == visible)
            return;

        QSignalBlocker blocker(ui->actionToolBar);   // 挡住信号，避免来回切换
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