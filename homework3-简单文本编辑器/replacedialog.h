#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class ReplaceDialog;
}

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog();

    void setFindText(const QString &text);
    QString findText() const;
    QString replaceText() const;
    bool caseSensitive() const;
    bool searchBackward() const;

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);
    void replaceRequested(const QString &text, const QString &replacement,
                          bool caseSensitive, bool backward);
    void replaceAllRequested(const QString &text, const QString &replacement,
                             bool caseSensitive, bool backward);

private slots:
    void onFindClicked();
    void onReplaceClicked();
    void onReplaceAllClicked();

private:
    Ui::ReplaceDialog *ui;
};

#endif
