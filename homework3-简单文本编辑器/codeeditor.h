#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QMouseEvent>
#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    int reservedWidth() const { return viewportMargins().left(); }
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void refreshLineNumberArea();

    void setLineNumbersVisible(bool visible);
    bool isLineNumbersVisible() const { return m_visible; }

    void selectLineAt(int y);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_visible = true;
};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }

    void mousePressEvent(QMouseEvent *event) override
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_editor->selectLineAt(event->position().toPoint().y());
#else
        m_editor->selectLineAt(event->pos().y());
#endif
        event->accept();
    }

private:
    CodeEditor *m_editor;
};

#endif
