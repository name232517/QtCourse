#pragma once

#include <QObject>

class NavigationController final : public QObject
{
    Q_OBJECT

public:
    enum class Page { Login, Workbench, Patients, Editor, Departments, Doctors, History };
    Q_ENUM(Page)

    Page currentPage() const;

    void enterWorkspace();
    void selectWorkspacePage(Page page);
    void goBack();
    void returnToLogin();

signals:
    void pageChanged(Page previous, Page current);

private:
    void setPage(Page page);

    Page currentPage_ = Page::Login;
};
