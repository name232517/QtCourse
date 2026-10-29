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

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(QColor(0x60, 0x60, 0x60));
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
