#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class FindDialog;
}

// 查找对话框：收集「查找什么、区分大小写吗、往上还是往下找」三个条件，
// 点「查找下一个」时用信号把它们交给主窗口，真正在编辑区里查找的是 MainWindow。
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog();

    void setFindText(const QString &text);   // 打开对话框时把编辑区里选中的文字带进来
    QString findText() const;                // 当前「查找目标」输入框里的内容
    bool caseSensitive() const;              // 是否区分大小写
    bool searchBackward() const;             // 是否向上查找

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);

private slots:
    void onFindClicked();                    // 「查找下一个」按钮或回车

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
