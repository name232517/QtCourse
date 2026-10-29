#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class LineNumberArea;

// 带行号栏的编辑区：继承 QPlainTextEdit，在左侧留出一条边，自己把行号画上去
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;          // 行号栏需要多宽（跟总行数位数、字体有关）
    int reservedWidth() const { return viewportMargins().left(); }   // 行号栏实际占掉的宽度
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void refreshLineNumberArea();             // 字体变化后重新算宽度

    void setLineNumbersVisible(bool visible);
    bool isLineNumbersVisible() const { return m_visible; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_visible = true;
};

// 行号栏本身：只负责把自己画出来，绘制交给 CodeEditor
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

private:
    CodeEditor *m_editor;
};

#endif // CODEEDITOR_H
