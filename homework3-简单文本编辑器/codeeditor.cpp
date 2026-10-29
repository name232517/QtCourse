#include "codeeditor.h"

#include <QFontMetrics>
#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
{
    // 行数变了要重算宽度；滚动或重绘时行号栏跟着刷新
    connect(this, &QPlainTextEdit::blockCountChanged, this,
            [this](int) { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);

    // 光标换行时要重画行号栏（把当前行的行号高亮挪过去）
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        if (m_visible)
            m_lineNumberArea->update();
    });

    updateLineNumberAreaWidth();
}

// 宽度 = 左右留白 + 最大行号的位数 × 一个数字的宽度
int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_visible)
        return 0;

    int digits = 1;
    int maxLines = qMax(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }

    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    m_lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth();
}

// 把行号栏挂到编辑区左侧：给 viewport 留出左边的边距
void CodeEditor::updateLineNumberAreaWidth()
{
    const int width = lineNumberAreaWidth();
    setViewportMargins(width, 0, 0, 0);

    // 边距一变，行号栏自己的位置也要跟着挪，否则会留出空白或压住文字
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
}

void CodeEditor::refreshLineNumberArea()
{
    updateLineNumberAreaWidth();
    if (m_visible)
        m_lineNumberArea->update();
}

// 点行号栏：把那一行整行选中，光标也放到行首，方便直接复制或删除
void CodeEditor::selectLineAt(int y)
{
    if (!m_visible)
        return;

    QTextCursor cursor = cursorForPosition(QPoint(0, y));
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
    setFocus();
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (!m_visible)
        return;

    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);   // 内容整体滚动，行号跟着滚即可
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0xFD, 0xF6, 0xE3));   // 淡黄底，与参考界面一致

    const int currentLine = textCursor().blockNumber();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool isCurrent = (blockNumber == currentLine);

            // 当前行：整条行号底色加深一点，数字也加深，方便一眼看到光标在哪一行
            if (isCurrent) {
                painter.fillRect(QRect(0, top, m_lineNumberArea->width(),
                                       qRound(blockBoundingRect(block).height())),
                                 QColor(0xF6, 0xE2, 0xB4));
            }

            painter.setPen(isCurrent ? QColor(0x20, 0x20, 0x20) : QColor(0x60, 0x60, 0x60));
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
