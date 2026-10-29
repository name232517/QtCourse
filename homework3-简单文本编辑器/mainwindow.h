#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class FindDialog;
class ReplaceDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 这些槽由 QMetaObject::connectSlotsByName 按 on_对象名_信号 的命名自动连接
    void on_actionFind_triggered();
    void on_actionReplace_triggered();
    void on_actionAbout_triggered();

private:
    void initStatusBar();   // 把 .ui 里放好的信息标签挂到状态栏右侧

    Ui::MainWindow *ui;

    // 查找、替换是非模态窗口，只创建一次，关掉以后再点菜单重新显示
    FindDialog *m_findDialog = nullptr;
    ReplaceDialog *m_replaceDialog = nullptr;
};

#endif // MAINWINDOW_H
