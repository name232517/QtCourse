#include "codeeditor.h"

#include <QFontMetrics>
#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>

namespace {

QColor blend(const QColor &from, const QColor &to, qreal ratio)
{
    return QColor::fromRgbF(from.redF() * (1.0 - ratio) + to.redF() * ratio,
                            from.greenF() * (1.0 - ratio) + to.greenF() * ratio,
                            from.blueF() * (1.0 - ratio) + to.blueF() * ratio);
}

}

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
{

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            [this](int) { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);

    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        if (m_visible)
            m_lineNumberArea->update();
    });

    updateLineNumberAreaWidth();
}

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

void CodeEditor::updateLineNumberAreaWidth()
{
    const int width = lineNumberAreaWidth();
    setViewportMargins(width, 0, 0, 0);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
}

void CodeEditor::refreshLineNumberArea()
{
    updateLineNumberAreaWidth();
    if (m_visible)
        m_lineNumberArea->update();
}

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
        m_lineNumberArea->scroll(0, dy);
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
    const QPalette pal = palette();
    const QColor baseColor = pal.color(QPalette::Base);
    const QColor textColor = pal.color(QPalette::Text);
    const QColor highlightColor = pal.color(QPalette::Highlight);

    const QColor gutterColor = blend(baseColor, textColor, 0.04);
    const QColor currentColor = blend(baseColor, highlightColor, 0.16);
    const QColor borderColor = blend(baseColor, textColor, 0.12);

    QColor numberColor = textColor;
    numberColor.setAlpha(140);
    QColor currentNumberColor = textColor;
    currentNumberColor.setAlpha(230);

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), gutterColor);

    const int currentLine = textCursor().blockNumber();
    const int width = m_lineNumberArea->width();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool isCurrent = (blockNumber == currentLine);
            const int height = qRound(blockBoundingRect(block).height());

            if (isCurrent)
                painter.fillRect(QRect(0, top, width, height), currentColor);

            painter.setPen(isCurrent ? currentNumberColor : numberColor);
            painter.drawText(0, top, width - 6, fontMetrics().height(),
                             Qt::AlignRight, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }

    painter.setPen(borderColor);
    painter.drawLine(width - 1, event->rect().top(), width - 1, event->rect().bottom());
}
