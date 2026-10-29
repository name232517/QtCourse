#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

namespace Ui {
class FindDialog;
}

// 查找对话框：只负责界面，查找逻辑在后续实验中接入
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog();

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
