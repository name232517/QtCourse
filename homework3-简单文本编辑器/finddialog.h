#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class FindDialog;
}

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog();

    void setFindText(const QString &text);
    QString findText() const;
    bool caseSensitive() const;
    bool searchBackward() const;

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);

private slots:
    void onFindClicked();

private:
    Ui::FindDialog *ui;
};

#endif
