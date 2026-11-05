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
class QColor;
class QTextCursor;
class QTimer;
class FindDialog;
class ReplaceDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:

    void on_actionFind_triggered();
    void on_actionReplace_triggered();
    void on_actionAbout_triggered();
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void updateWindowTitle();

    void on_actionFont_triggered();
    void on_actionFontColor_triggered();
    void on_actionFontBackground_triggered();
    void on_actionEditorBackground_triggered();
    void on_actionWrap_toggled(bool checked);
    void on_actionLineNumbers_toggled(bool checked);

    void on_actionToolBar_toggled(bool checked);
    void on_actionStatusBar_toggled(bool checked);
    void updateStatusInfo();

    void autoSave();

    void findNext(const QString &text, bool caseSensitive, bool backward);

    void onReplaceFindRequested(const QString &text, bool caseSensitive, bool backward);

    void replaceOne(const QString &text, const QString &replacement,
                    bool caseSensitive, bool backward);

    void replaceAll(const QString &text, const QString &replacement,
                    bool caseSensitive, bool backward);

private:
    void initStatusBar();
    void initViewActions();
    void initEditActions();
    void updateEditActions();
    void applyTextColor(const QColor &color);
    void applyTextBackground(const QColor &color);
    void applyEditorBackground(const QColor &color);
    bool maybeSave();
    bool saveFile();
    bool saveFileAs();
    bool writeFile(const QString &path, bool autoSaved = false);
    bool loadFile(const QString &path);
    void setCurrentFile(const QString &path);
    void initAutoSave();

    QTextCursor findMatch(const QString &text, bool caseSensitive, bool backward,
                          bool *wrapped) const;
    void bringToFront(QWidget *dialog);

    Ui::MainWindow *ui;

    FindDialog *m_findDialog = nullptr;
    ReplaceDialog *m_replaceDialog = nullptr;

    QString m_filePath;
    QString m_lastDir;

    QTimer *m_autoSaveTimer = nullptr;
};

#endif
