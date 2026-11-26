#include "previewwindow.h"

#include "database.h"
#include "patientsexdelegate.h"
#include "patienttablemodel.h"
#include "selectmodel.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QTableView>
#include <QtConcurrentRun>
#include <QVBoxLayout>
#include <QVector>

using Page = NavigationController::Page;
using PatientColumn = PatientTableModel::Column;

namespace {
enum class Icon { Clinic, Home, Patients, Search, Add, Staff, Log };

QIcon interfaceIcon(Icon icon)
{
    QPixmap pixmap(40, 40);
    pixmap.setDevicePixelRatio(2);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    if (icon == Icon::Clinic) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#235e56"));
        painter.drawRoundedRect(QRectF(0, 0, 20, 20), 4, 4);
    }
    painter.setBrush(Qt::NoBrush);
    const QColor ink = icon == Icon::Clinic || icon == Icon::Add ? QColor(Qt::white)
                                                                 : QColor("#455c5b");
    painter.setPen(QPen(ink, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    switch (icon) {
    case Icon::Clinic:
    case Icon::Add:
        painter.drawLine(QPointF(5, 10), QPointF(15, 10));
        painter.drawLine(QPointF(10, 5), QPointF(10, 15));
        break;
    case Icon::Home: {
        QPainterPath roof;
        roof.moveTo(2, 9);
        roof.lineTo(10, 3);
        roof.lineTo(18, 9);
        roof.moveTo(4, 8);
        roof.lineTo(4, 17);
        roof.lineTo(16, 17);
        roof.lineTo(16, 8);
        painter.drawPath(roof);
        break;
    }
    case Icon::Patients:
        painter.drawRoundedRect(QRectF(3, 4, 14, 12), 1.5, 1.5);
        painter.drawEllipse(QPointF(7.5, 9), 1.25, 1.25);
        painter.drawLine(QPointF(6, 12.5), QPointF(9, 12.5));
        painter.drawLine(QPointF(11.5, 9), QPointF(14, 9));
        painter.drawLine(QPointF(11.5, 12.5), QPointF(14, 12.5));
        break;
    case Icon::Search:
        painter.drawEllipse(QPointF(8.5, 8.5), 4.5, 4.5);
        painter.drawLine(QPointF(12, 12), QPointF(16, 16));
        break;
    case Icon::Staff:
        painter.drawEllipse(QPointF(10, 6.5), 3, 3);
        painter.drawArc(QRectF(4, 10, 12, 11), 0, 180 * 16);
        break;
    case Icon::Log:
        for (int line = 0; line < 3; ++line) {
            const qreal y = 5 + line * 5;
            painter.drawEllipse(QPointF(4.5, y), 1, 1);
            painter.drawLine(QPointF(8, y), QPointF(16, y));
        }
        break;
    }
    painter.end();
    return QIcon(pixmap);
}

QLabel *label(const QString &text, const char *style = nullptr)
{
    auto *widget = new QLabel(text);
    if (style) {
        widget->setObjectName(QString::fromLatin1(style));
    }
    return widget;
}

QPushButton *button(const QString &text, const char *role = "secondary")
{
    auto *widget = new QPushButton(text);
    widget->setProperty("role", QString::fromLatin1(role));
    return widget;
}

QHBoxLayout *productLabel()
{
    auto *row = new QHBoxLayout;
    row->setSpacing(9);
    auto *mark = new QLabel;
    mark->setPixmap(interfaceIcon(Icon::Clinic).pixmap(QSize(28, 28), 2.0));
    row->addWidget(mark);
    row->addWidget(label(QStringLiteral("诊疗工作台"), "productName"));
    return row;
}

QFrame *divider()
{
    auto *line = new QFrame;
    line->setObjectName(QStringLiteral("divider"));
    line->setFixedHeight(1);
    return line;
}

QVBoxLayout *pageLayout(QWidget *page)
{
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 22, 32, 22);
    layout->setSpacing(14);
    return layout;
}

void addField(QGridLayout *form, int row, int column, const QString &text, QWidget *input)
{
    auto *field = new QVBoxLayout;
    field->setSpacing(7);
    auto *caption = label(text, "fieldLabel");
    caption->setBuddy(input);
    input->setAccessibleName(text);
    field->addWidget(caption);
    field->addWidget(input);
    form->addLayout(field, row, column);
}

bool readMeasure(const QLineEdit *field, std::optional<double> &measure)
{
    const QString text = field->text().trimmed();
    if (text.isEmpty()) {
        measure.reset();
        return true;
    }
    bool parsed = false;
    measure = text.toDouble(&parsed);
    return parsed;
}

// The three read-only pages share the same table dressing as the patient list.
void dressTable(QTableView *table, const QString &objectName, const QString &accessibleName)
{
    table->setObjectName(objectName);
    table->setAccessibleName(accessibleName);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(37);
    auto *header = table->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setMinimumSectionSize(64);
}

struct DepartmentOption
{
    QString id;
    QString name;
};

QVector<DepartmentOption> departmentOptions(const Database &database)
{
    QVector<DepartmentOption> options;
    options.append({QString(), QStringLiteral("未分配")});
    QSqlQuery query(database.connection());
    if (query.exec(QStringLiteral("SELECT ID, NAME FROM Department ORDER BY NAME"))) {
        while (query.next()) {
            options.append({query.value(0).toString(), query.value(1).toString()});
        }
    }
    return options;
}
}

PreviewWindow::PreviewWindow(const Database &database, PatientTableModel &patientData, QWidget *parent)
    : QWidget(parent), database_(database), patientData_(patientData)
{
    setWindowTitle(QStringLiteral("诊疗工作台"));
    setWindowIcon(interfaceIcon(Icon::Clinic));
    resize(1160, 750);
    setMinimumSize(940, 660);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    root_ = new QStackedWidget;
    loginPage_ = createLoginPage();
    workspace_ = createWorkspace();
    root_->addWidget(loginPage_);
    root_->addWidget(workspace_);
    layout->addWidget(root_);
    connect(&navigation_, &NavigationController::pageChanged, this, &PreviewWindow::applyPage);
    username_->setFocus();
}

void PreviewWindow::closeEvent(QCloseEvent *event)
{
    if (confirmLeaveEditor()) {
        QWidget::closeEvent(event);
    } else {
        event->ignore();
    }
}

QWidget *PreviewWindow::createLoginPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("loginPage"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    auto *signInPanel = new QFrame;
    signInPanel->setObjectName(QStringLiteral("loginPanel"));
    signInPanel->setFixedWidth(390);
    auto *form = new QVBoxLayout(signInPanel);
    form->setContentsMargins(28, 26, 28, 24);
    form->setSpacing(0);
    layout->addStretch(2);
    layout->addWidget(signInPanel, 0, Qt::AlignHCenter);
    layout->addStretch(3);

    auto *brand = productLabel();
    brand->addStretch();
    form->addLayout(brand);
    form->addSpacing(19);
    form->addWidget(label(QStringLiteral("登录"), "pageTitle"));
    form->addSpacing(17);

    username_ = new QLineEdit;
    username_->setObjectName(QStringLiteral("username"));
    username_->setMaxLength(64);
    password_ = new QLineEdit;
    password_->setObjectName(QStringLiteral("password"));
    password_->setEchoMode(QLineEdit::Password);

    auto *fields = new QGridLayout;
    fields->setContentsMargins(0, 0, 0, 0);
    fields->setVerticalSpacing(13);
    addField(fields, 0, 0, QStringLiteral("用户名"), username_);
    addField(fields, 1, 0, QStringLiteral("密码"), password_);
    form->addLayout(fields);
    form->addSpacing(18);

    signIn_ = button(QStringLiteral("登录"), "primary");
    signIn_->setObjectName(QStringLiteral("signIn"));
    form->addWidget(signIn_);
    form->addSpacing(10);
    loginFeedback_ = label({}, "loginFeedback");
    loginFeedback_->setMinimumHeight(20);
    loginFeedback_->setWordWrap(true);
    form->addWidget(loginFeedback_);
    connect(signIn_, &QPushButton::clicked, this, &PreviewWindow::attemptLogin);
    connect(username_, &QLineEdit::returnPressed, password_, [this] { password_->setFocus(); });
    connect(password_, &QLineEdit::returnPressed, this, &PreviewWindow::attemptLogin);
    connect(username_, &QLineEdit::textEdited, loginFeedback_, &QLabel::clear);
    connect(password_, &QLineEdit::textEdited, loginFeedback_, &QLabel::clear);
    connect(&loginCheck_, &QFutureWatcher<PasswordDigest::Check>::finished, this, [this] {
        finishLogin(loginCheck_.result());
    });
    return page;
}

QWidget *PreviewWindow::createWorkspace()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("workspace"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *appBar = new QFrame;
    appBar->setObjectName(QStringLiteral("appBar"));
    auto *bar = new QHBoxLayout(appBar);
    bar->setContentsMargins(32, 8, 32, 8);
    bar->setSpacing(5);
    bar->addLayout(productLabel());
    bar->addSpacing(22);

    homeNav_ = button(QStringLiteral("工作台"), "nav");
    patientNav_ = button(QStringLiteral("患者管理"), "nav");
    departmentNav_ = button(QStringLiteral("科室管理"), "nav");
    doctorNav_ = button(QStringLiteral("医生管理"), "nav");
    historyNav_ = button(QStringLiteral("操作记录"), "nav");
    homeNav_->setObjectName(QStringLiteral("homeNav"));
    patientNav_->setObjectName(QStringLiteral("patientNav"));
    departmentNav_->setObjectName(QStringLiteral("departmentNav"));
    doctorNav_->setObjectName(QStringLiteral("doctorNav"));
    historyNav_->setObjectName(QStringLiteral("historyNav"));
    homeNav_->setIcon(interfaceIcon(Icon::Home));
    patientNav_->setIcon(interfaceIcon(Icon::Patients));
    departmentNav_->setIcon(interfaceIcon(Icon::Patients));
    doctorNav_->setIcon(interfaceIcon(Icon::Staff));
    historyNav_->setIcon(interfaceIcon(Icon::Log));
    for (auto *action : {homeNav_, patientNav_, departmentNav_, doctorNav_, historyNav_}) {
        action->setCheckable(true);
        action->setAutoExclusive(true);
        action->setIconSize(QSize(16, 16));
        bar->addWidget(action);
    }
    bar->addStretch();
    auto *exit = button(QStringLiteral("退出登录"), "quiet");
    exit->setObjectName(QStringLiteral("signOut"));
    bar->addWidget(exit);

    pages_ = new QStackedWidget;
    homePage_ = createHomePage();
    patientPage_ = createPatientPage();
    editorPage_ = createEditorPage();
    departmentPage_ = createDepartmentPage();
    doctorPage_ = createDoctorPage();
    historyPage_ = createHistoryPage();
    pages_->addWidget(homePage_);
    pages_->addWidget(patientPage_);
    pages_->addWidget(editorPage_);
    pages_->addWidget(departmentPage_);
    pages_->addWidget(doctorPage_);
    pages_->addWidget(historyPage_);
    layout->addWidget(appBar);
    layout->addWidget(pages_, 1);

    connect(homeNav_, &QPushButton::clicked, this, [this] {
        if (navigation_.currentPage() == Page::Editor) {
            patientNav_->setChecked(true);
        }
        if (confirmLeaveEditor()) {
            navigation_.selectWorkspacePage(Page::Workbench);
        }
    });
    connect(patientNav_, &QPushButton::clicked, this, [this] {
        if (confirmLeaveEditor()) {
            navigation_.selectWorkspacePage(Page::Patients);
        }
    });
    const auto openModule = [this](Page page, QPushButton *nav) {
        return [this, page, nav] {
            if (!confirmLeaveEditor()) {
                return;
            }
            nav->setChecked(true);
            navigation_.selectWorkspacePage(page);
        };
    };
    connect(departmentNav_, &QPushButton::clicked, this, openModule(Page::Departments, departmentNav_));
    connect(doctorNav_, &QPushButton::clicked, this, openModule(Page::Doctors, doctorNav_));
    connect(historyNav_, &QPushButton::clicked, this, openModule(Page::History, historyNav_));
    connect(exit, &QPushButton::clicked, this, [this] {
        if (confirmLeaveEditor()) {
            logEvent(QStringLiteral("退出登录"));
            navigation_.returnToLogin();
        }
    });

    auto *backShortcut = new QShortcut(QKeySequence::Back, page);
    backShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(backShortcut, &QShortcut::activated, this, &PreviewWindow::goBack);
    return page;
}

QWidget *PreviewWindow::createHomePage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("homePage"));
    auto *layout = pageLayout(page);
    layout->addWidget(label(QStringLiteral("工作台"), "pageTitle"));

    auto *modules = new QFrame;
    modules->setObjectName(QStringLiteral("panel"));
    auto *list = new QVBoxLayout(modules);
    list->setContentsMargins(18, 4, 18, 4);
    list->setSpacing(0);

    auto *patient = new QHBoxLayout;
    patient->setContentsMargins(0, 12, 0, 12);
    patient->addWidget(label(QStringLiteral("患者管理"), "sectionTitle"));
    patient->addStretch();
    auto *open = button(QStringLiteral("打开患者管理"));
    open->setObjectName(QStringLiteral("openPatients"));
    patient->addWidget(open);
    list->addLayout(patient);
    const struct {
        QString title;
        QString action;
        const char *objectName;
        Page page;
    } modulesList[] = {
        {QStringLiteral("医生管理"), QStringLiteral("打开医生管理"), "openDoctors", Page::Doctors},
        {QStringLiteral("科室管理"), QStringLiteral("打开科室管理"), "openDepartments", Page::Departments},
        {QStringLiteral("操作记录"), QStringLiteral("查看操作记录"), "openHistory", Page::History},
    };
    for (const auto &module : modulesList) {
        list->addWidget(divider());
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 12, 0, 12);
        row->addWidget(label(module.title, "sectionTitle"));
        row->addStretch();
        auto *openModulePage = button(module.action);
        openModulePage->setObjectName(QString::fromLatin1(module.objectName));
        row->addWidget(openModulePage);
        list->addLayout(row);
        connect(openModulePage, &QPushButton::clicked, &navigation_, [this, page = module.page] {
            navigation_.selectWorkspacePage(page);
        });
    }
    layout->addWidget(modules);
    layout->addStretch();
    connect(open, &QPushButton::clicked, &navigation_, [this] {
        navigation_.selectWorkspacePage(Page::Patients);
    });
    return page;
}

QWidget *PreviewWindow::createPatientPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("patientPage"));
    auto *layout = pageLayout(page);

    auto *top = new QHBoxLayout;
    top->addWidget(label(QStringLiteral("患者管理"), "pageTitle"), 1);
    auto *add = button(QStringLiteral("新增患者"), "primary");
    add->setObjectName(QStringLiteral("addPatient"));
    add->setIcon(interfaceIcon(Icon::Add));
    add->setIconSize(QSize(15, 15));
    top->addWidget(add, 0, Qt::AlignVCenter);
    layout->addLayout(top);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *body = new QVBoxLayout(panel);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(12, 10, 12, 10);
    toolbar->setSpacing(6);
    search_ = new QLineEdit;
    search_->setObjectName(QStringLiteral("patientSearch"));
    search_->setPlaceholderText(QStringLiteral("姓名、病历编号等"));
    search_->setAccessibleName(QStringLiteral("搜索患者"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumWidth(220);
    search_->setMaximumWidth(322);
    auto *runSearch = button(QStringLiteral("查询"));
    runSearch->setObjectName(QStringLiteral("runSearch"));
    runSearch->setIcon(interfaceIcon(Icon::Search));
    runSearch->setIconSize(QSize(15, 15));
    auto *findShortcut = new QShortcut(QKeySequence::Find, page);
    findShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(findShortcut, &QShortcut::activated, search_, [this] {
        search_->setFocus();
        search_->selectAll();
    });
    editAction_ = button(QStringLiteral("修改"));
    editAction_->setObjectName(QStringLiteral("editPatient"));
    deleteAction_ = button(QStringLiteral("删除"), "danger");
    deleteAction_->setObjectName(QStringLiteral("deletePatient"));
    toolbar->addWidget(search_, 1);
    toolbar->addWidget(runSearch);
    toolbar->addStretch();
    toolbar->addWidget(editAction_);
    toolbar->addWidget(deleteAction_);
    body->addLayout(toolbar);

    patients_ = new QSortFilterProxyModel(page);
    patients_->setSourceModel(&patientData_);
    patients_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    patients_->setFilterKeyColumn(-1);

    patientTable_ = new QTableView;
    patientTable_->setObjectName(QStringLiteral("patientTable"));
    patientTable_->setAccessibleName(QStringLiteral("患者列表"));
    patientTable_->setModel(patients_);
    patientTable_->setEditTriggers(QAbstractItemView::EditKeyPressed);
    patientTable_->setItemDelegateForColumn(PatientColumn::Sex, new PatientSexDelegate(patientTable_));
    patientTable_->setContextMenuPolicy(Qt::CustomContextMenu);
    patientTable_->setToolTip(QStringLiteral("双击打开档案。右键可复制病历编号或修改性别。"));
    patientTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    patientTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    patientTable_->setAlternatingRowColors(true);
    patientTable_->setShowGrid(false);
    patientTable_->verticalHeader()->hide();
    patientTable_->verticalHeader()->setDefaultSectionSize(37);
    auto *header = patientTable_->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setMinimumSectionSize(64);
    configurePatientColumns();

    auto *empty = new QWidget;
    auto *emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setSpacing(8);
    emptyLayout->addStretch();
    emptyTitle_ = label({}, "sectionTitle");
    emptyDescription_ = label({}, "muted");
    emptyDescription_->setTextFormat(Qt::PlainText);
    emptyDescription_->setWordWrap(true);
    emptyDescription_->setAlignment(Qt::AlignHCenter);
    emptyDescription_->setFixedWidth(520);
    clearSearch_ = button(QStringLiteral("查看全部"));
    clearSearch_->setObjectName(QStringLiteral("clearSearch"));
    emptyLayout->addWidget(emptyTitle_, 0, Qt::AlignHCenter);
    emptyLayout->addWidget(emptyDescription_, 0, Qt::AlignHCenter);
    emptyLayout->addWidget(clearSearch_, 0, Qt::AlignHCenter);
    emptyLayout->addStretch();

    patientResults_ = new QStackedWidget;
    patientResults_->addWidget(patientTable_);
    patientResults_->addWidget(empty);
    body->addWidget(patientResults_, 1);

    auto *foot = new QFrame;
    foot->setObjectName(QStringLiteral("tableFooter"));
    auto *footer = new QHBoxLayout(foot);
    footer->setContentsMargins(12, 7, 12, 7);
    patientCount_ = label({}, "muted");
    patientCount_->setObjectName(QStringLiteral("patientCount"));
    footer->addWidget(patientCount_);
    patientNotice_ = label({}, "muted");
    patientNotice_->setObjectName(QStringLiteral("patientNotice"));
    patientNotice_->setTextFormat(Qt::PlainText);
    footer->addWidget(patientNotice_, 1, Qt::AlignCenter);
    footer->addWidget(label(QStringLiteral("SQLite 已连接"), "muted"));
    body->addWidget(foot);
    layout->addWidget(panel, 1);

    connect(runSearch, &QPushButton::clicked, this, &PreviewWindow::filterPatients);
    connect(search_, &QLineEdit::returnPressed, this, &PreviewWindow::filterPatients);
    connect(search_, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.isEmpty()) {
            filterPatients();
        }
    });
    connect(clearSearch_, &QPushButton::clicked, this, [this] {
        if (patientLoadFailed_) {
            reloadPatients();
        } else {
            search_->clear();
        }
        search_->setFocus();
    });
    connect(patientTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PreviewWindow::updatePatientActions);
    auto *copyShortcut = new QShortcut(QKeySequence::Copy, patientTable_);
    copyShortcut->setContext(Qt::WidgetShortcut);
    connect(copyShortcut, &QShortcut::activated, this, [this] {
        copyPatientId(patientTable_->selectionModel()->selectedRows(PatientColumn::Id).value(0));
    });
    connect(patientTable_, &QTableView::customContextMenuRequested, this, [this](const QPoint &position) {
        const QPersistentModelIndex cell(patientTable_->indexAt(position).siblingAtColumn(PatientColumn::Sex));
        if (!cell.isValid()) {
            return;
        }
        QMenu menu(patientTable_);
        auto *copyId = menu.addAction(QStringLiteral("复制病历编号"));
        auto *editSex = menu.addAction(QStringLiteral("修改性别"));
        const QAction *chosen = menu.exec(patientTable_->viewport()->mapToGlobal(position));
        if (!cell.isValid()) {
            return;
        }
        if (chosen == copyId) {
            copyPatientId(cell.sibling(cell.row(), PatientColumn::Id));
        } else if (chosen == editSex) {
            patientTable_->setCurrentIndex(cell);
            patientTable_->edit(cell);
        }
    });
    // Reload after the delegate has closed its editor.
    connect(&patientData_, &PatientTableModel::patientSaved,
            this, &PreviewWindow::refreshSavedPatient, Qt::QueuedConnection);
    connect(&patientData_, &PatientTableModel::patientSaved, this, [this](const QString &id) {
        logEvent(QStringLiteral("修改患者 %1 的性别").arg(id));
    }, Qt::QueuedConnection);
    connect(&patientData_, &PatientTableModel::patientSaveFailed, this, [this](const QString &error) {
        QMessageBox dialog(QMessageBox::Warning, QStringLiteral("无法保存患者档案"), {}, QMessageBox::Close, this);
        dialog.setTextFormat(Qt::PlainText);
        dialog.setText(error);
        dialog.button(QMessageBox::Close)->setText(QStringLiteral("关闭"));
        dialog.exec();
    }, Qt::QueuedConnection);
    connect(editAction_, &QPushButton::clicked, this, [this] {
        showEditor(patientTable_->selectionModel()->selectedRows().value(0));
    });
    connect(patientTable_, &QTableView::doubleClicked, this, &PreviewWindow::showEditor);
    connect(add, &QPushButton::clicked, this, [this] { showEditor(QModelIndex()); });
    connect(deleteAction_, &QPushButton::clicked, this, &PreviewWindow::deleteSelectedPatient);
    updatePatientResults();
    return page;
}

QWidget *PreviewWindow::createEditorPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("editorPage"));
    auto *layout = pageLayout(page);

    auto *top = new QHBoxLayout;
    editorTitle_ = label({}, "pageTitle");
    top->addWidget(editorTitle_, 1);
    auto *back = button(QStringLiteral("返回列表"), "quiet");
    back->setObjectName(QStringLiteral("backToPatients"));
    top->addWidget(back, 0, Qt::AlignVCenter);
    layout->addLayout(top);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 15, 20, 17);
    panelLayout->setSpacing(13);
    auto *formCaption = new QHBoxLayout;
    formCaption->addWidget(label(QStringLiteral("档案信息"), "sectionTitle"));
    formCaption->addStretch();
    formCaption->addWidget(label(QStringLiteral("* 为必填项"), "muted"));
    panelLayout->addLayout(formCaption);

    auto *fields = new QGridLayout;
    fields->setHorizontalSpacing(24);
    fields->setVerticalSpacing(10);
    fields->setColumnStretch(0, 1);
    fields->setColumnStretch(1, 1);

    patientName_ = new QLineEdit;
    patientName_->setObjectName(QStringLiteral("patientName"));
    patientName_->setMaxLength(128);
    identityCard_ = new QLineEdit;
    identityCard_->setObjectName(QStringLiteral("patientIdentityCard"));
    identityCard_->setMaxLength(40);
    sex_ = new QComboBox;
    sex_->setObjectName(QStringLiteral("patientSex"));
    sex_->addItem(QStringLiteral("请选择"));
    for (int value = 0; value <= 2; ++value) {
        sex_->addItem(PatientRecord::sexLabel(value), value);
    }
    birthDate_ = new QDateEdit;
    birthDate_->setObjectName(QStringLiteral("patientBirthDate"));
    birthDate_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    birthDate_->setCalendarPopup(true);
    birthDate_->setKeyboardTracking(false);
    birthDate_->setDateRange(QDate(100, 1, 1), QDate::currentDate());
    birthDate_->setSpecialValueText(QStringLiteral("选择出生日期"));
    mobile_ = new QLineEdit;
    mobile_->setObjectName(QStringLiteral("patientMobile"));
    mobile_->setMaxLength(40);
    patientId_ = new QLineEdit;
    patientId_->setObjectName(QStringLiteral("patientId"));
    patientId_->setReadOnly(true);
    patientId_->setPlaceholderText(QStringLiteral("保存时自动生成"));
    createdTimestamp_ = new QLineEdit;
    createdTimestamp_->setObjectName(QStringLiteral("patientCreatedTimestamp"));
    createdTimestamp_->setReadOnly(true);
    createdTimestamp_->setPlaceholderText(QStringLiteral("保存时自动记录"));
    height_ = new QLineEdit;
    height_->setObjectName(QStringLiteral("patientHeight"));
    height_->setPlaceholderText(QStringLiteral("选填"));
    weight_ = new QLineEdit;
    weight_->setObjectName(QStringLiteral("patientWeight"));
    weight_->setPlaceholderText(QStringLiteral("选填"));

    addField(fields, 0, 0, QStringLiteral("姓名 *"), patientName_);
    addField(fields, 0, 1, QStringLiteral("性别 *"), sex_);
    addField(fields, 1, 0, QStringLiteral("证件号码 *"), identityCard_);
    addField(fields, 1, 1, QStringLiteral("出生日期 *"), birthDate_);
    addField(fields, 2, 0, QStringLiteral("手机号码"), mobile_);
    addField(fields, 3, 0, QStringLiteral("病历编号"), patientId_);
    addField(fields, 3, 1, QStringLiteral("创建时间（本地）"), createdTimestamp_);
    panelLayout->addLayout(fields);
    panelLayout->addWidget(divider());
    panelLayout->addWidget(label(QStringLiteral("体征记录"), "sectionTitle"));
    auto *measures = new QGridLayout;
    measures->setHorizontalSpacing(24);
    measures->setColumnStretch(0, 1);
    measures->setColumnStretch(1, 1);
    addField(measures, 0, 0, QStringLiteral("身高（cm）"), height_);
    addField(measures, 0, 1, QStringLiteral("体重（kg）"), weight_);
    panelLayout->addLayout(measures);
    layout->addWidget(panel);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(9);
    editorNotice_ = label({}, "editorFeedback");
    editorNotice_->setTextFormat(Qt::PlainText);
    editorNotice_->setWordWrap(true);
    actions->addWidget(editorNotice_, 1);
    auto *cancel = button(QStringLiteral("取消"));
    cancel->setObjectName(QStringLiteral("cancelEditor"));
    auto *save = button(QStringLiteral("保存"), "primary");
    save->setObjectName(QStringLiteral("savePatient"));
    actions->addWidget(cancel);
    actions->addWidget(save);
    layout->addLayout(actions);
    layout->addStretch();
    connect(back, &QPushButton::clicked, this, &PreviewWindow::goBack);
    connect(cancel, &QPushButton::clicked, this, &PreviewWindow::goBack);
    connect(save, &QPushButton::clicked, this, &PreviewWindow::saveEditor);
    return page;
}

QWidget *PreviewWindow::createDepartmentPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("departmentPage"));
    auto *layout = pageLayout(page);

    auto *top = new QHBoxLayout;
    top->addWidget(label(QStringLiteral("科室管理"), "pageTitle"), 1);
    auto *add = button(QStringLiteral("新增科室"), "primary");
    add->setObjectName(QStringLiteral("addDepartment"));
    add->setIcon(interfaceIcon(Icon::Add));
    add->setIconSize(QSize(15, 15));
    top->addWidget(add, 0, Qt::AlignVCenter);
    layout->addLayout(top);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *body = new QVBoxLayout(panel);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(12, 10, 12, 10);
    toolbar->setSpacing(6);
    departmentNotice_ = label({}, "muted");
    departmentNotice_->setObjectName(QStringLiteral("departmentNotice"));
    departmentNotice_->setTextFormat(Qt::PlainText);
    renameDepartmentAction_ = button(QStringLiteral("重命名"));
    renameDepartmentAction_->setObjectName(QStringLiteral("renameDepartment"));
    deleteDepartmentAction_ = button(QStringLiteral("删除"), "danger");
    deleteDepartmentAction_->setObjectName(QStringLiteral("deleteDepartment"));
    toolbar->addWidget(departmentNotice_, 1);
    toolbar->addWidget(renameDepartmentAction_);
    toolbar->addWidget(deleteDepartmentAction_);
    body->addLayout(toolbar);

    departmentData_ = new SelectModel(database_, page);
    departmentTable_ = new QTableView;
    dressTable(departmentTable_, QStringLiteral("departmentTable"), QStringLiteral("科室列表"));
    departmentTable_->setModel(departmentData_);
    departmentTable_->setToolTip(QStringLiteral("选中一行后可重命名或删除；仍有医生时不允许删除。"));
    body->addWidget(departmentTable_, 1);

    auto *foot = new QFrame;
    foot->setObjectName(QStringLiteral("tableFooter"));
    auto *footer = new QHBoxLayout(foot);
    footer->setContentsMargins(12, 7, 12, 7);
    departmentCount_ = label({}, "muted");
    departmentCount_->setObjectName(QStringLiteral("departmentCount"));
    footer->addWidget(departmentCount_);
    footer->addStretch();
    footer->addWidget(label(QStringLiteral("SQLite 已连接"), "muted"));
    body->addWidget(foot);
    layout->addWidget(panel, 1);

    auto updateActions = [this] {
        const bool selected = departmentTable_->selectionModel()->hasSelection();
        renameDepartmentAction_->setEnabled(selected);
        deleteDepartmentAction_->setEnabled(selected);
    };
    connect(departmentTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, updateActions);
    connect(departmentTable_, &QTableView::doubleClicked, this, &PreviewWindow::renameDepartment);
    connect(add, &QPushButton::clicked, this, &PreviewWindow::addDepartment);
    connect(renameDepartmentAction_, &QPushButton::clicked, this, &PreviewWindow::renameDepartment);
    connect(deleteDepartmentAction_, &QPushButton::clicked, this, &PreviewWindow::deleteDepartment);
    updateActions();
    return page;
}

QWidget *PreviewWindow::createDoctorPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("doctorPage"));
    auto *layout = pageLayout(page);

    auto *top = new QHBoxLayout;
    top->addWidget(label(QStringLiteral("医生管理"), "pageTitle"), 1);
    auto *add = button(QStringLiteral("新增医生"), "primary");
    add->setObjectName(QStringLiteral("addDoctor"));
    add->setIcon(interfaceIcon(Icon::Add));
    add->setIconSize(QSize(15, 15));
    top->addWidget(add, 0, Qt::AlignVCenter);
    layout->addLayout(top);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *body = new QVBoxLayout(panel);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(12, 10, 12, 10);
    toolbar->setSpacing(6);
    doctorNotice_ = label({}, "muted");
    doctorNotice_->setObjectName(QStringLiteral("doctorNotice"));
    doctorNotice_->setTextFormat(Qt::PlainText);
    editDoctorAction_ = button(QStringLiteral("修改"));
    editDoctorAction_->setObjectName(QStringLiteral("editDoctor"));
    deleteDoctorAction_ = button(QStringLiteral("删除"), "danger");
    deleteDoctorAction_->setObjectName(QStringLiteral("deleteDoctor"));
    toolbar->addWidget(doctorNotice_, 1);
    toolbar->addWidget(editDoctorAction_);
    toolbar->addWidget(deleteDoctorAction_);
    body->addLayout(toolbar);

    doctorData_ = new SelectModel(database_, page);
    doctorTable_ = new QTableView;
    dressTable(doctorTable_, QStringLiteral("doctorTable"), QStringLiteral("医生列表"));
    doctorTable_->setModel(doctorData_);
    doctorTable_->setToolTip(QStringLiteral("工号唯一；科室可以留空表示尚未分配。"));
    body->addWidget(doctorTable_, 1);

    auto *foot = new QFrame;
    foot->setObjectName(QStringLiteral("tableFooter"));
    auto *footer = new QHBoxLayout(foot);
    footer->setContentsMargins(12, 7, 12, 7);
    doctorCount_ = label({}, "muted");
    doctorCount_->setObjectName(QStringLiteral("doctorCount"));
    footer->addWidget(doctorCount_);
    footer->addStretch();
    footer->addWidget(label(QStringLiteral("SQLite 已连接"), "muted"));
    body->addWidget(foot);
    layout->addWidget(panel, 1);

    auto updateActions = [this] {
        const bool selected = doctorTable_->selectionModel()->hasSelection();
        editDoctorAction_->setEnabled(selected);
        deleteDoctorAction_->setEnabled(selected);
    };
    connect(doctorTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this, updateActions);
    connect(doctorTable_, &QTableView::doubleClicked, this, &PreviewWindow::editDoctor);
    connect(add, &QPushButton::clicked, this, &PreviewWindow::addDoctor);
    connect(editDoctorAction_, &QPushButton::clicked, this, &PreviewWindow::editDoctor);
    connect(deleteDoctorAction_, &QPushButton::clicked, this, &PreviewWindow::deleteDoctor);
    updateActions();
    return page;
}

QWidget *PreviewWindow::createHistoryPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("historyPage"));
    auto *layout = pageLayout(page);

    auto *top = new QHBoxLayout;
    top->addWidget(label(QStringLiteral("操作记录"), "pageTitle"), 1);
    auto *refresh = button(QStringLiteral("刷新"));
    refresh->setObjectName(QStringLiteral("refreshHistory"));
    top->addWidget(refresh, 0, Qt::AlignVCenter);
    layout->addLayout(top);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *body = new QVBoxLayout(panel);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(12, 10, 12, 10);
    toolbar->setSpacing(6);
    toolbar->addWidget(label(QStringLiteral("登录、退出与患者、科室、医生的增删改都会记录在这里。"), "muted"), 1);
    body->addLayout(toolbar);

    historyData_ = new SelectModel(database_, page);
    historyTable_ = new QTableView;
    dressTable(historyTable_, QStringLiteral("historyTable"), QStringLiteral("操作记录列表"));
    historyTable_->setModel(historyData_);
    body->addWidget(historyTable_, 1);

    auto *foot = new QFrame;
    foot->setObjectName(QStringLiteral("tableFooter"));
    auto *footer = new QHBoxLayout(foot);
    footer->setContentsMargins(12, 7, 12, 7);
    historyCount_ = label({}, "muted");
    historyCount_->setObjectName(QStringLiteral("historyCount"));
    footer->addWidget(historyCount_);
    historyNotice_ = label({}, "muted");
    historyNotice_->setObjectName(QStringLiteral("historyNotice"));
    footer->addWidget(historyNotice_, 1, Qt::AlignCenter);
    footer->addWidget(label(QStringLiteral("SQLite 已连接"), "muted"));
    body->addWidget(foot);
    layout->addWidget(panel, 1);

    connect(refresh, &QPushButton::clicked, this, &PreviewWindow::reloadHistory);
    return page;
}

void PreviewWindow::logEvent(const QString &event)
{
    QString error;
    if (!database_.appendHistory(currentUserId_, event, error) && historyNotice_) {
        historyNotice_->setText(QStringLiteral("操作记录未能保存：%1").arg(error));
    }
}

void PreviewWindow::reloadDepartments()
{
    QString error;
    const bool loaded = departmentData_->load(QStringLiteral(
        "SELECT d.NAME AS 名称, d.ID AS 科室编号, "
        "(SELECT COUNT(*) FROM Doctor w WHERE w.DEPARTMENT_ID = d.ID) AS 医生数 "
        "FROM Department d ORDER BY d.NAME"), error);
    if (!loaded) {
        departmentCount_->setText(QStringLiteral("记录数不可用"));
        departmentNotice_->setText(QStringLiteral("无法加载科室列表：%1").arg(error));
        return;
    }
    departmentTable_->hideColumn(1);
    departmentTable_->clearSelection();
    departmentTable_->setCurrentIndex(QModelIndex());
    const int count = departmentData_->rowCount();
    departmentCount_->setText(QStringLiteral("%1 个科室").arg(count));
    renameDepartmentAction_->setEnabled(false);
    deleteDepartmentAction_->setEnabled(false);
}

void PreviewWindow::reloadDoctors()
{
    QString error;
    const bool loaded = doctorData_->load(QStringLiteral(
        "SELECT w.EMPLOYEENO AS 工号, w.NAME AS 姓名, COALESCE(d.NAME, '未分配') AS 所属科室, "
        "w.DEPARTMENT_ID AS 科室编号, w.ID AS 记录编号 "
        "FROM Doctor w LEFT JOIN Department d ON d.ID = w.DEPARTMENT_ID "
        "ORDER BY w.EMPLOYEENO"), error);
    if (!loaded) {
        doctorCount_->setText(QStringLiteral("记录数不可用"));
        doctorNotice_->setText(QStringLiteral("无法加载医生列表：%1").arg(error));
        return;
    }
    doctorTable_->hideColumn(3);
    doctorTable_->hideColumn(4);
    doctorTable_->clearSelection();
    doctorTable_->setCurrentIndex(QModelIndex());
    doctorCount_->setText(QStringLiteral("%1 位医生").arg(doctorData_->rowCount()));
    editDoctorAction_->setEnabled(false);
    deleteDoctorAction_->setEnabled(false);
}

void PreviewWindow::reloadHistory()
{
    QString error;
    const bool loaded = historyData_->load(QStringLiteral(
        "SELECT strftime('%Y-%m-%d %H:%M:%S', h.TIMESTAMP, 'localtime') AS 时间, "
        "COALESCE(u.USERNAME, '系统') AS 账号, h.EVENT AS 操作内容 "
        "FROM History h LEFT JOIN \"User\" u ON u.ID = h.USER_ID "
        "ORDER BY h.TIMESTAMP DESC"), error);
    if (!loaded) {
        historyCount_->setText(QStringLiteral("记录数不可用"));
        historyNotice_->setText(QStringLiteral("无法加载操作记录：%1").arg(error));
        return;
    }
    historyTable_->clearSelection();
    historyTable_->setCurrentIndex(QModelIndex());
    historyNotice_->clear();
    historyCount_->setText(QStringLiteral("%1 条操作记录").arg(historyData_->rowCount()));
}

void PreviewWindow::addDepartment()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新增科室"),
                                               QStringLiteral("科室名称（1 至 64 个字符）："),
                                               QLineEdit::Normal, {}, &accepted).trimmed();
    if (!accepted) {
        return;
    }

    DepartmentRecord record;
    record.name = name;
    QString error;
    const auto result = database_.saveDepartment(record, std::nullopt, error);
    reloadDepartments();
    if (result == Database::DepartmentSave::Saved) {
        departmentNotice_->setText(QStringLiteral("已新增科室 %1。").arg(record.name));
        logEvent(QStringLiteral("新增科室 %1").arg(record.name));
    } else {
        departmentNotice_->setText(error);
    }
}

void PreviewWindow::renameDepartment()
{
    const QModelIndex cell = departmentTable_->selectionModel()->selectedRows().value(0);
    if (!cell.isValid()) {
        return;
    }
    const QString id = departmentData_->index(cell.row(), 1).data(Qt::EditRole).toString();
    const QString previousName = departmentData_->index(cell.row(), 0).data(Qt::EditRole).toString();

    bool accepted = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("重命名科室"),
                                               QStringLiteral("科室名称（1 至 64 个字符）："),
                                               QLineEdit::Normal, previousName, &accepted).trimmed();
    if (!accepted) {
        return;
    }

    DepartmentRecord record;
    record.id = id;
    record.name = name;
    DepartmentRecord original;
    original.id = id;
    original.name = previousName;
    QString error;
    const auto result = database_.saveDepartment(record, original, error);
    reloadDepartments();
    if (result == Database::DepartmentSave::Saved) {
        departmentNotice_->setText(QStringLiteral("已把“%1”改名为“%2”。").arg(previousName, record.name));
        logEvent(QStringLiteral("科室改名：%1 → %2").arg(previousName, record.name));
    } else {
        departmentNotice_->setText(error);
    }
}

void PreviewWindow::deleteDepartment()
{
    const QModelIndex cell = departmentTable_->selectionModel()->selectedRows().value(0);
    if (!cell.isValid()) {
        return;
    }
    const QString id = departmentData_->index(cell.row(), 1).data(Qt::EditRole).toString();
    const QString name = departmentData_->index(cell.row(), 0).data(Qt::EditRole).toString();

    QMessageBox confirmation(QMessageBox::Warning, QStringLiteral("删除科室"), {},
                             QMessageBox::Yes | QMessageBox::Cancel, this);
    confirmation.setTextFormat(Qt::PlainText);
    confirmation.setText(QStringLiteral("删除科室“%1”？").arg(name));
    confirmation.button(QMessageBox::Yes)->setText(QStringLiteral("删除"));
    confirmation.button(QMessageBox::Yes)->setProperty("role", QStringLiteral("danger"));
    confirmation.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
    confirmation.setDefaultButton(QMessageBox::Cancel);
    confirmation.setEscapeButton(QMessageBox::Cancel);
    if (confirmation.exec() != QMessageBox::Yes) {
        return;
    }

    QString error;
    const auto result = database_.removeDepartment(id, error);
    reloadDepartments();
    reloadDoctors();
    switch (result) {
    case Database::DepartmentRemoval::Removed:
        departmentNotice_->setText(QStringLiteral("已删除科室“%1”。").arg(name));
        logEvent(QStringLiteral("删除科室 %1").arg(name));
        break;
    case Database::DepartmentRemoval::Missing:
        departmentNotice_->setText(QStringLiteral("该科室已不存在。"));
        break;
    case Database::DepartmentRemoval::InUse:
    case Database::DepartmentRemoval::Error:
        departmentNotice_->setText(error);
        break;
    }
}

void PreviewWindow::addDoctor()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新增医生"));
    auto *form = new QFormLayout(&dialog);
    auto *number = new QLineEdit(&dialog);
    number->setObjectName(QStringLiteral("doctorEmployeeNo"));
    number->setMaxLength(32);
    auto *name = new QLineEdit(&dialog);
    name->setObjectName(QStringLiteral("doctorName"));
    name->setMaxLength(64);
    auto *department = new QComboBox(&dialog);
    department->setObjectName(QStringLiteral("doctorDepartment"));
    for (const auto &option : departmentOptions(database_)) {
        department->addItem(option.name, option.id);
    }
    form->addRow(QStringLiteral("工号 *"), number);
    form->addRow(QStringLiteral("姓名 *"), name);
    form->addRow(QStringLiteral("所属科室"), department);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    DoctorRecord record;
    record.employeeNo = number->text();
    record.name = name->text();
    record.departmentId = department->currentData().toString();
    QString error;
    const auto result = database_.saveDoctor(record, std::nullopt, error);
    reloadDoctors();
    reloadDepartments();
    if (result == Database::DoctorSave::Saved) {
        doctorNotice_->setText(QStringLiteral("已新增医生 %1（工号 %2）。").arg(record.name, record.employeeNo));
        logEvent(QStringLiteral("新增医生 %1（工号 %2）").arg(record.name, record.employeeNo));
    } else {
        doctorNotice_->setText(error);
    }
}

void PreviewWindow::editDoctor()
{
    const QModelIndex cell = doctorTable_->selectionModel()->selectedRows().value(0);
    if (!cell.isValid()) {
        return;
    }
    DoctorRecord original;
    original.employeeNo = doctorData_->index(cell.row(), 0).data(Qt::EditRole).toString();
    original.name = doctorData_->index(cell.row(), 1).data(Qt::EditRole).toString();
    original.departmentId = doctorData_->index(cell.row(), 3).data(Qt::EditRole).toString();
    original.id = doctorData_->index(cell.row(), 4).data(Qt::EditRole).toString();

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("修改医生"));
    auto *form = new QFormLayout(&dialog);
    auto *number = new QLineEdit(original.employeeNo, &dialog);
    number->setObjectName(QStringLiteral("doctorEmployeeNo"));
    number->setMaxLength(32);
    auto *name = new QLineEdit(original.name, &dialog);
    name->setObjectName(QStringLiteral("doctorName"));
    name->setMaxLength(64);
    auto *department = new QComboBox(&dialog);
    department->setObjectName(QStringLiteral("doctorDepartment"));
    for (const auto &option : departmentOptions(database_)) {
        department->addItem(option.name, option.id);
    }
    const int index = department->findData(original.departmentId);
    department->setCurrentIndex(index >= 0 ? index : 0);
    form->addRow(QStringLiteral("工号 *"), number);
    form->addRow(QStringLiteral("姓名 *"), name);
    form->addRow(QStringLiteral("所属科室"), department);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    DoctorRecord updated;
    updated.id = original.id;
    updated.employeeNo = number->text();
    updated.name = name->text();
    updated.departmentId = department->currentData().toString();
    QString error;
    const auto result = database_.saveDoctor(updated, original, error);
    reloadDoctors();
    reloadDepartments();
    if (result == Database::DoctorSave::Saved) {
        doctorNotice_->setText(QStringLiteral("已保存医生 %1（工号 %2）。").arg(updated.name, updated.employeeNo));
        logEvent(QStringLiteral("修改医生 %1（工号 %2）").arg(updated.name, updated.employeeNo));
    } else {
        doctorNotice_->setText(error);
    }
}

void PreviewWindow::deleteDoctor()
{
    const QModelIndex cell = doctorTable_->selectionModel()->selectedRows().value(0);
    if (!cell.isValid()) {
        return;
    }
    const QString id = doctorData_->index(cell.row(), 4).data(Qt::EditRole).toString();
    const QString name = doctorData_->index(cell.row(), 1).data(Qt::EditRole).toString();
    const QString number = doctorData_->index(cell.row(), 0).data(Qt::EditRole).toString();

    QMessageBox confirmation(QMessageBox::Warning, QStringLiteral("删除医生"), {},
                             QMessageBox::Yes | QMessageBox::Cancel, this);
    confirmation.setTextFormat(Qt::PlainText);
    confirmation.setText(QStringLiteral("删除医生“%1”（工号 %2）？").arg(name, number));
    confirmation.button(QMessageBox::Yes)->setText(QStringLiteral("删除"));
    confirmation.button(QMessageBox::Yes)->setProperty("role", QStringLiteral("danger"));
    confirmation.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
    confirmation.setDefaultButton(QMessageBox::Cancel);
    confirmation.setEscapeButton(QMessageBox::Cancel);
    if (confirmation.exec() != QMessageBox::Yes) {
        return;
    }

    QString error;
    const auto result = database_.removeDoctor(id, error);
    reloadDoctors();
    reloadDepartments();
    if (result == Database::DoctorRemoval::Removed) {
        doctorNotice_->setText(QStringLiteral("已删除医生“%1”。").arg(name));
        logEvent(QStringLiteral("删除医生 %1（工号 %2）").arg(name, number));
    } else if (result == Database::DoctorRemoval::Missing) {
        doctorNotice_->setText(QStringLiteral("该医生记录已不存在。"));
    } else {
        doctorNotice_->setText(error);
    }
}

void PreviewWindow::attemptLogin()
{
    if (navigation_.currentPage() != Page::Login || !signIn_->isEnabled()) {
        return;
    }

    const QString username = username_->text().trimmed();
    if (username.isEmpty()) {
        loginFeedback_->setText(QStringLiteral("请输入用户名。"));
        username_->setFocus();
        return;
    }
    if (password_->text().isEmpty()) {
        loginFeedback_->setText(QStringLiteral("请输入密码。"));
        password_->setFocus();
        return;
    }

    QByteArray digest;
    QString userId;
    const auto credential = database_.passwordDigestForUser(username, digest, &userId);
    if (credential != Database::CredentialLookup::Found) {
        finishLogin(credential == Database::CredentialLookup::UnknownUser
            ? PasswordDigest::Check::Mismatch : PasswordDigest::Check::Unavailable);
        return;
    }
    pendingUserId_ = userId;
    currentUserName_ = username;

    const QByteArray password = password_->text().toUtf8();
    password_->clear();
    loginFeedback_->clear();
    username_->setEnabled(false);
    password_->setEnabled(false);
    signIn_->setEnabled(false);
    signIn_->setText(QStringLiteral("正在验证…"));
    // The SQLite connection stays on the UI thread. Only the digest calculation goes to the pool.
    loginCheck_.setFuture(QtConcurrent::run(PasswordDigest::verify, password, digest));
}

void PreviewWindow::finishLogin(PasswordDigest::Check result)
{
    username_->setEnabled(true);
    password_->setEnabled(true);
    signIn_->setEnabled(true);
    signIn_->setText(QStringLiteral("登录"));
    password_->clear();

    if (result == PasswordDigest::Check::Match) {
        loginFeedback_->clear();
        currentUserId_ = pendingUserId_;
        pendingUserId_.clear();
        logEvent(QStringLiteral("登录系统"));
        navigation_.enterWorkspace();
        return;
    }
    pendingUserId_.clear();

    loginFeedback_->setText(result == PasswordDigest::Check::Mismatch
        ? QStringLiteral("用户名或密码错误。")
        : QStringLiteral("无法验证账号数据，请检查数据库。"));
    password_->setFocus();
}

void PreviewWindow::applyPage(Page previous, Page current)
{
    QWidget *focus = nullptr;
    switch (current) {
    case Page::Login:
        root_->setCurrentWidget(loginPage_);
        clearWorkspace();
        username_->setFocus();
        return;
    case Page::Workbench:
        pages_->setCurrentWidget(homePage_);
        homeNav_->setChecked(true);
        focus = homeNav_;
        break;
    case Page::Patients:
        if (previous == Page::Workbench) {
            patientNotice_->clear();
            reloadPatients();
        }
        pages_->setCurrentWidget(patientPage_);
        patientNav_->setChecked(true);
        focus = previous == Page::Editor && patientTable_->selectionModel()->hasSelection()
            ? static_cast<QWidget *>(patientTable_) : search_;
        break;
    case Page::Editor:
        pages_->setCurrentWidget(editorPage_);
        patientNav_->setChecked(true);
        focus = patientName_;
        break;
    case Page::Departments:
        if (previous != Page::Departments) {
            departmentNotice_->clear();
            reloadDepartments();
        }
        pages_->setCurrentWidget(departmentPage_);
        departmentNav_->setChecked(true);
        focus = departmentTable_;
        break;
    case Page::Doctors:
        if (previous != Page::Doctors) {
            doctorNotice_->clear();
            reloadDoctors();
        }
        pages_->setCurrentWidget(doctorPage_);
        doctorNav_->setChecked(true);
        focus = doctorTable_;
        break;
    case Page::History:
        if (previous != Page::History) {
            reloadHistory();
        }
        pages_->setCurrentWidget(historyPage_);
        historyNav_->setChecked(true);
        focus = historyTable_;
        break;
    }

    root_->setCurrentWidget(workspace_);
    focus->setFocus();
}

void PreviewWindow::clearWorkspace()
{
    currentUserId_.clear();
    currentUserName_.clear();
    username_->clear();
    password_->clear();
    loginFeedback_->clear();
    {
        const QSignalBlocker blocker(search_);
        search_->clear();
    }
    patients_->setFilterFixedString({});
    patientNotice_->clear();
    patientTable_->selectionModel()->clear();
    patientTable_->scrollToTop();
    populateEditor(QModelIndex());
}

void PreviewWindow::filterPatients()
{
    patientNotice_->clear();
    patients_->setFilterFixedString(search_->text().trimmed());
    reloadPatients();
}

void PreviewWindow::reloadPatients()
{
    patientTable_->selectionModel()->clear();
    const bool restoreColumns = patients_->columnCount() == 0;
    QString error;
    if (patientData_.load(error) && restoreColumns) {
        configurePatientColumns();
    }
    updatePatientResults(error);
}

void PreviewWindow::updatePatientResults(const QString &error)
{
    patientLoadFailed_ = !error.isEmpty();
    const int count = patients_->rowCount();
    const bool filtered = !patients_->filterRegularExpression().pattern().isEmpty();
    if (patientLoadFailed_) {
        emptyTitle_->setText(QStringLiteral("无法加载患者列表"));
        emptyDescription_->setText(error);
        clearSearch_->setText(QStringLiteral("重试查询"));
        clearSearch_->show();
    } else if (count == 0) {
        emptyTitle_->setText(filtered ? QStringLiteral("未找到患者") : QStringLiteral("暂无患者记录"));
        emptyDescription_->setText(filtered ? QStringLiteral("请检查关键词或查看全部患者。")
                                             : QStringLiteral("当前数据库中没有患者档案。"));
        clearSearch_->setText(QStringLiteral("查看全部"));
        clearSearch_->setVisible(filtered);
    }
    patientResults_->setCurrentIndex(!patientLoadFailed_ && count > 0 ? 0 : 1);
    const QString countText = filtered ? QStringLiteral("匹配 %1 条患者记录")
                                      : QStringLiteral("%1 条患者记录");
    patientCount_->setText(patientLoadFailed_ ? QStringLiteral("记录数不可用") : countText.arg(count));
    updatePatientActions();
}

void PreviewWindow::configurePatientColumns()
{
    auto *header = patientTable_->horizontalHeader();
    header->setSectionResizeMode(PatientColumn::Id, QHeaderView::Stretch);
    header->resizeSection(PatientColumn::Name, 96);
    header->resizeSection(PatientColumn::Sex, 64);
    header->resizeSection(PatientColumn::BirthDate, 105);
    header->resizeSection(PatientColumn::IdentityCard, 155);
    header->resizeSection(PatientColumn::Mobile, 112);
    header->resizeSection(PatientColumn::CreatedTimestamp, 166);
    patientTable_->hideColumn(PatientColumn::Height);
    patientTable_->hideColumn(PatientColumn::Weight);
}

void PreviewWindow::updatePatientActions()
{
    const bool selected = !patientLoadFailed_ && patientTable_->selectionModel()->hasSelection();
    editAction_->setEnabled(selected);
    deleteAction_->setEnabled(selected);
}

void PreviewWindow::copyPatientId(const QModelIndex &patient)
{
    if (!patient.isValid()) {
        return;
    }
    QApplication::clipboard()->setText(patient.data(Qt::EditRole).toString());
    patientNotice_->setText(QStringLiteral("已复制病历编号。"));
}

void PreviewWindow::deleteSelectedPatient()
{
    const QModelIndex row = patientTable_->selectionModel()->selectedRows(PatientColumn::Id).value(0);
    if (navigation_.currentPage() != Page::Patients || !row.isValid()) {
        return;
    }

    const QString id = row.data(Qt::EditRole).toString();
    const QString name = row.siblingAtColumn(PatientColumn::Name).data(Qt::DisplayRole).toString();
    QMessageBox confirmation(QMessageBox::Warning, QStringLiteral("删除患者"), {},
                             QMessageBox::Yes | QMessageBox::Cancel, this);
    confirmation.setTextFormat(Qt::PlainText);
    confirmation.setText(QStringLiteral("删除患者“%1”（病历编号 %2）？\n此操作无法撤销。").arg(name, id));
    confirmation.button(QMessageBox::Yes)->setText(QStringLiteral("删除"));
    confirmation.button(QMessageBox::Yes)->setProperty("role", QStringLiteral("danger"));
    confirmation.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
    confirmation.setDefaultButton(QMessageBox::Cancel);
    confirmation.setEscapeButton(QMessageBox::Cancel);
    if (confirmation.exec() != QMessageBox::Yes) {
        return;
    }

    patientNotice_->clear();
    QString error;
    const auto result = database_.removePatient(id, error);
    if (result == Database::PatientRemoval::Error) {
        QMessageBox failure(QMessageBox::Critical, QStringLiteral("删除患者失败"), {},
                            QMessageBox::Close, this);
        failure.setTextFormat(Qt::PlainText);
        failure.setText(QStringLiteral("删除未完成。\n%1").arg(error));
        failure.button(QMessageBox::Close)->setText(QStringLiteral("关闭"));
        failure.exec();
        return;
    }

    reloadPatients();
    patientNotice_->setText(result == Database::PatientRemoval::Removed
        ? QStringLiteral("已删除患者 %1。").arg(id)
        : QStringLiteral("该患者记录已不存在。"));
    if (result == Database::PatientRemoval::Removed) {
        logEvent(QStringLiteral("删除患者 %1").arg(id));
    }
    search_->setFocus();
}

void PreviewWindow::showEditor(const QModelIndex &patient)
{
    if (navigation_.currentPage() != Page::Patients) {
        return;
    }

    populateEditor(patient);
    navigation_.selectWorkspacePage(Page::Editor);
}

void PreviewWindow::populateEditor(const QModelIndex &patient)
{
    const QModelIndex source = patients_->mapToSource(patient);
    originalPatient_ = source.isValid() ? std::optional<PatientRecord>(patientData_.patientAt(source.row()))
                                        : std::nullopt;
    const bool editing = originalPatient_.has_value();
    const PatientRecord record = originalPatient_.value_or(PatientRecord{});
    editorTitle_->setText(editing ? QStringLiteral("编辑患者") : QStringLiteral("新增患者"));
    editorNotice_->clear();
    patientId_->setText(record.id);
    createdTimestamp_->setText(source.siblingAtColumn(PatientColumn::CreatedTimestamp).data().toString());
    patientName_->setText(record.name);
    identityCard_->setText(record.identityCard);
    sex_->setCurrentIndex(editing ? sex_->findData(record.sex) : 0);
    const QDate birthDate = QDate::fromString(record.birthDate, Qt::ISODate);
    birthDate_->setMaximumDate(qMax(QDate::currentDate(), birthDate));
    birthDate_->setDate(birthDate.isValid() ? birthDate : birthDate_->minimumDate());
    mobile_->setText(record.mobile);
    const auto displayMeasure = [](const std::optional<double> &value) {
        return value ? QLocale::c().toString(*value, 'g', QLocale::FloatingPointShortest) : QString();
    };
    height_->setText(displayMeasure(record.height));
    weight_->setText(displayMeasure(record.weight));
    initialEditorInput_ = editorInput();
}

QStringList PreviewWindow::editorInput() const
{
    return {patientName_->text(), identityCard_->text(), QString::number(sex_->currentIndex()),
            birthDate_->text(), mobile_->text(), height_->text(), weight_->text()};
}

bool PreviewWindow::confirmLeaveEditor()
{
    if (navigation_.currentPage() != Page::Editor || initialEditorInput_ == editorInput()) {
        return true;
    }

    QMessageBox confirmation(QMessageBox::Question, QStringLiteral("放弃未保存的修改？"),
                             QStringLiteral("离开后，本次填写的内容将丢失。"),
                             QMessageBox::Discard | QMessageBox::Cancel, this);
    confirmation.button(QMessageBox::Discard)->setText(QStringLiteral("放弃修改"));
    confirmation.button(QMessageBox::Cancel)->setText(QStringLiteral("继续编辑"));
    confirmation.setDefaultButton(QMessageBox::Cancel);
    confirmation.setEscapeButton(QMessageBox::Cancel);
    return confirmation.exec() == QMessageBox::Discard;
}

void PreviewWindow::goBack()
{
    if (confirmLeaveEditor()) {
        navigation_.goBack();
    }
}

void PreviewWindow::saveEditor()
{
    if (navigation_.currentPage() != Page::Editor) {
        return;
    }

    PatientRecord updated = originalPatient_.value_or(PatientRecord{});
    const auto editedText = [](const QLineEdit *field, const QString &previous) {
        const QString text = field->text();
        return text == previous ? previous : text.trimmed();
    };
    updated.name = editedText(patientName_, updated.name);
    updated.identityCard = editedText(identityCard_, updated.identityCard);
    updated.sex = sex_->currentData().isValid() ? sex_->currentData().toInt() : -1;
    const QDate date = birthDate_->date();
    if (date != birthDate_->minimumDate()) {
        updated.birthDate = date.toString(Qt::ISODate);
    } else if (QDate::fromString(updated.birthDate, Qt::ISODate) > birthDate_->minimumDate()) {
        updated.birthDate.clear();
    }
    if (mobile_->text() != updated.mobile) {
        const QString mobile = mobile_->text().trimmed();
        updated.mobile = mobile.isEmpty() ? QString() : mobile;
    }
    if (!readMeasure(height_, updated.height)) {
        editorNotice_->setText(QStringLiteral("身高应为大于零的数字，或留空。"));
        height_->setFocus();
        return;
    }
    if (!readMeasure(weight_, updated.weight)) {
        editorNotice_->setText(QStringLiteral("体重应为大于零的数字，或留空。"));
        weight_->setFocus();
        return;
    }

    QString error;
    if (database_.savePatient(updated, originalPatient_, error) != Database::PatientSave::Saved) {
        editorNotice_->setText(error);
        return;
    }
    logEvent(originalPatient_
        ? QStringLiteral("修改患者 %1").arg(updated.id)
        : QStringLiteral("新增患者 %1").arg(updated.id));
    refreshSavedPatient(updated.id);
    navigation_.goBack();
}

void PreviewWindow::refreshSavedPatient(const QString &id)
{
    reloadPatients();
    const auto matches = patients_->match(patients_->index(0, PatientColumn::Id), Qt::EditRole,
                                         id, 1, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        const QModelIndex match = matches.first();
        patientTable_->setCurrentIndex(match);
        patientTable_->selectRow(match.row());
        patientTable_->scrollTo(match);
    }
    patientNotice_->setText(matches.isEmpty() && !patientLoadFailed_
        ? QStringLiteral("已保存患者 %1，当前筛选中未显示。").arg(id)
        : QStringLiteral("已保存患者 %1。").arg(id));
}
