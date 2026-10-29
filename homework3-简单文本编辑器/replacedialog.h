#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class ReplaceDialog;
}

// 替换对话框：和查找对话框一样，只负责收集条件，点按钮时把条件用信号交给主窗口，
// 由 MainWindow 去编辑区里查找、替换。
class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog();

    void setFindText(const QString &text);   // 打开对话框时把编辑区选中的文字带进来
    QString findText() const;                // 查找目标
    QString replaceText() const;             // 替换为
    bool caseSensitive() const;              // 是否区分大小写
    bool searchBackward() const;             // 是否向上

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);
    void replaceRequested(const QString &text, const QString &replacement,
                          bool caseSensitive, bool backward);
    void replaceAllRequested(const QString &text, const QString &replacement,
                             bool caseSensitive, bool backward);

private slots:
    void onFindClicked();       // 查找下一个
    void onReplaceClicked();    // 替换
    void onReplaceAllClicked(); // 全部替换

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H
