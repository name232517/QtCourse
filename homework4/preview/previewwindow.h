#pragma once

#include "navigationcontroller.h"

#include <QWidget>

class PatientTableModel;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
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
    explicit PreviewWindow(PatientTableModel &patientData, QWidget *parent = nullptr);

private:
    QWidget *createLoginPage();
    QWidget *createWorkspace(PatientTableModel &patientData);
    QWidget *createHomePage();
    QWidget *createPatientPage(PatientTableModel &patientData);
    QWidget *createEditorPage();
    void applyPage(NavigationController::Page previous, NavigationController::Page current);
    void clearWorkspace();
    void filterPatients();
    void updatePatientActions();
    void showEditor(const QModelIndex &patient);
    void populateEditor(const QModelIndex &patient);

    NavigationController navigation_;
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
    QLineEdit *search_ = nullptr;
    QTableView *patientTable_ = nullptr;
    QSortFilterProxyModel *patients_ = nullptr;
    QStackedWidget *patientResults_ = nullptr;
    QLabel *patientCount_ = nullptr;
    QLabel *emptyTitle_ = nullptr;
    QLabel *emptyDescription_ = nullptr;
    QPushButton *clearSearch_ = nullptr;
    QPushButton *editAction_ = nullptr;
    QPushButton *deleteAction_ = nullptr;

    QLabel *editorTitle_ = nullptr;
    QLabel *editorNotice_ = nullptr;
    QLineEdit *patientId_ = nullptr;
    QLineEdit *patientName_ = nullptr;
    QLineEdit *identityCard_ = nullptr;
    QComboBox *sex_ = nullptr;
    QDateEdit *birthDate_ = nullptr;
    QLineEdit *mobile_ = nullptr;
    QDoubleSpinBox *height_ = nullptr;
    QDoubleSpinBox *weight_ = nullptr;
};
