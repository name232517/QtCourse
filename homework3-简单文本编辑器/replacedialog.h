#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

namespace Ui {
class ReplaceDialog;
}

// 替换对话框：只负责界面，替换逻辑在后续实验中接入
class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog();

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H
