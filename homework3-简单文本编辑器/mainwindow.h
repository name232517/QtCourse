#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class FindDialog;
class ReplaceDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;   // 关窗口前处理未保存的修改

private slots:
    // 这些槽由 QMetaObject::connectSlotsByName 按 on_对象名_信号 的命名自动连接
    void on_actionFind_triggered();
    void on_actionReplace_triggered();
    void on_actionAbout_triggered();
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void updateWindowTitle();                       // 标题随文件名与修改状态变化

private:
    void initStatusBar();                           // 把 .ui 里放好的信息标签挂到状态栏右侧
    bool maybeSave();                               // 有未保存修改时询问「保存 / 不保存 / 取消」
    bool saveFile();                                // 保存到当前文件；还没有路径时转为另存为
    bool saveFileAs();                              // 选择路径后保存
    bool writeFile(const QString &path);            // 真正写盘
    bool loadFile(const QString &path);             // 读盘并显示到编辑区
    void setCurrentFile(const QString &path);       // 更新当前文件路径与标题

    Ui::MainWindow *ui;

    // 查找、替换是非模态窗口，只创建一次，关掉以后再点菜单重新显示
    FindDialog *m_findDialog = nullptr;
    ReplaceDialog *m_replaceDialog = nullptr;

    QString m_filePath;   // 当前文件的完整路径，空表示还没保存过
    QString m_lastDir;    // 上一次打开/保存所在的目录
};

#endif // MAINWINDOW_H
