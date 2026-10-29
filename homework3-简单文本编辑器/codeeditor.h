#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QMouseEvent>
#include <QPlainTextEdit>

class LineNumberArea;

// 自定义的可显示行号的文本编辑控件：
// 继承 QPlainTextEdit，在左侧留出一条边（viewportMargins），由一个子控件 LineNumberArea 负责绘制行号。
// 在 Qt Designer 里通过「提升（promote）」把 QPlainTextEdit 换成这个类即可使用。
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;          // 行号栏需要多宽（跟总行数位数、字体有关）
    int reservedWidth() const { return viewportMargins().left(); }   // 行号栏实际占掉的宽度
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void refreshLineNumberArea();             // 字体变化后重新算宽度并重绘

    void setLineNumbersVisible(bool visible);
    bool isLineNumbersVisible() const { return m_visible; }

    void selectLineAt(int y);                 // 点击行号栏时选中那一整行

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_visible = true;
};

// 行号栏本身：只负责接事件、把自己画出来，真正的绘制交给 CodeEditor
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

    // 点行号栏 = 选中这一行
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

#endif // CODEEDITOR_H
