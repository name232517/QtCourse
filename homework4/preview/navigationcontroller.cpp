#include "navigationcontroller.h"

NavigationController::Page NavigationController::currentPage() const
{
    return currentPage_;
}

void NavigationController::enterWorkspace()
{
    if (currentPage_ == Page::Login) {
        setPage(Page::Workbench);
    }
}

void NavigationController::selectWorkspacePage(Page page)
{
    if (currentPage_ == Page::Login) {
        return;
    }

    switch (page) {
    case Page::Workbench:
    case Page::Patients:
    case Page::Departments:
    case Page::Doctors:
    case Page::History:
        setPage(page);
        break;
    case Page::Editor:
        if (currentPage_ == Page::Patients) {
            setPage(page);
        }
        break;
    case Page::Login:
        break;
    }
}

void NavigationController::goBack()
{
    if (currentPage_ == Page::Editor) {
        setPage(Page::Patients);
    } else if (currentPage_ == Page::Patients || currentPage_ == Page::Departments
               || currentPage_ == Page::Doctors || currentPage_ == Page::History) {
        setPage(Page::Workbench);
    }
}

void NavigationController::returnToLogin()
{
    setPage(Page::Login);
}

void NavigationController::setPage(Page page)
{
    if (page == currentPage_) {
        return;
    }

    const Page previous = currentPage_;
    currentPage_ = page;
    emit pageChanged(previous, currentPage_);
}
