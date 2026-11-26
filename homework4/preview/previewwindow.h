#pragma once

#include "navigationcontroller.h"
#include "passworddigest.h"
#include "patientrecord.h"
#include "staffrecord.h"

#include <QFutureWatcher>
#include <QStringList>
#include <QWidget>

class Database;
class PatientTableModel;
class SelectModel;
class QCloseEvent;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QStackedWidget;
class QTableView;

class PreviewWindow final : public QWidget
{
public:
    PreviewWindow(const Database &database, PatientTableModel &patientData, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *createLoginPage();
    QWidget *createWorkspace();
    QWidget *createHomePage();
    QWidget *createPatientPage();
    QWidget *createEditorPage();
    QWidget *createDepartmentPage();
    QWidget *createDoctorPage();
    QWidget *createHistoryPage();
    void attemptLogin();
    void finishLogin(PasswordDigest::Check result);
    void applyPage(NavigationController::Page previous, NavigationController::Page current);
    void clearWorkspace();
    void filterPatients();
    void reloadPatients();
    void updatePatientResults(const QString &error = {});
    void configurePatientColumns();
    void deleteSelectedPatient();
    void copyPatientId(const QModelIndex &patient);
    void updatePatientActions();
    void showEditor(const QModelIndex &patient);
    void populateEditor(const QModelIndex &patient);
    QStringList editorInput() const;
    bool confirmLeaveEditor();
    void goBack();
    void saveEditor();
    void refreshSavedPatient(const QString &id);
    void logEvent(const QString &event);
    void reloadDepartments();
    void reloadDoctors();
    void reloadHistory();
    void addDepartment();
    void renameDepartment();
    void deleteDepartment();
    void addDoctor();
    void editDoctor();
    void deleteDoctor();

    const Database &database_;
    PatientTableModel &patientData_;
    NavigationController navigation_;
    QFutureWatcher<PasswordDigest::Check> loginCheck_;
    QStackedWidget *root_ = nullptr;
    QWidget *loginPage_ = nullptr;
    QWidget *workspace_ = nullptr;
    QStackedWidget *pages_ = nullptr;
    QWidget *homePage_ = nullptr;
    QWidget *patientPage_ = nullptr;
    QWidget *editorPage_ = nullptr;
    QPushButton *homeNav_ = nullptr;
    QPushButton *patientNav_ = nullptr;

    QLineEdit *username_ = nullptr;
    QLineEdit *password_ = nullptr;
    QPushButton *signIn_ = nullptr;
    QLabel *loginFeedback_ = nullptr;
    QLineEdit *search_ = nullptr;
    QTableView *patientTable_ = nullptr;
    QSortFilterProxyModel *patients_ = nullptr;
    QStackedWidget *patientResults_ = nullptr;
    QLabel *patientCount_ = nullptr;
    QLabel *patientNotice_ = nullptr;
    QLabel *emptyTitle_ = nullptr;
    QLabel *emptyDescription_ = nullptr;
    QPushButton *clearSearch_ = nullptr;
    QPushButton *editAction_ = nullptr;
    QPushButton *deleteAction_ = nullptr;
    bool patientLoadFailed_ = false;

    QLabel *editorTitle_ = nullptr;
    QLabel *editorNotice_ = nullptr;
    QLineEdit *patientId_ = nullptr;
    QLineEdit *createdTimestamp_ = nullptr;
    QLineEdit *patientName_ = nullptr;
    QLineEdit *identityCard_ = nullptr;
    QComboBox *sex_ = nullptr;
    QDateEdit *birthDate_ = nullptr;
    QLineEdit *mobile_ = nullptr;
    QLineEdit *height_ = nullptr;
    QLineEdit *weight_ = nullptr;
    std::optional<PatientRecord> originalPatient_;
    QStringList initialEditorInput_;

    QPushButton *departmentNav_ = nullptr;
    QPushButton *doctorNav_ = nullptr;
    QPushButton *historyNav_ = nullptr;
    QWidget *departmentPage_ = nullptr;
    QWidget *doctorPage_ = nullptr;
    QWidget *historyPage_ = nullptr;
    SelectModel *departmentData_ = nullptr;
    SelectModel *doctorData_ = nullptr;
    SelectModel *historyData_ = nullptr;
    QTableView *departmentTable_ = nullptr;
    QTableView *doctorTable_ = nullptr;
    QTableView *historyTable_ = nullptr;
    QLabel *departmentCount_ = nullptr;
    QLabel *doctorCount_ = nullptr;
    QLabel *historyCount_ = nullptr;
    QLabel *departmentNotice_ = nullptr;
    QLabel *doctorNotice_ = nullptr;
    QLabel *historyNotice_ = nullptr;
    QPushButton *renameDepartmentAction_ = nullptr;
    QPushButton *deleteDepartmentAction_ = nullptr;
    QPushButton *editDoctorAction_ = nullptr;
    QPushButton *deleteDoctorAction_ = nullptr;
    QString currentUserId_;
    QString currentUserName_;
    QString pendingUserId_;
};
