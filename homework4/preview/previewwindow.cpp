#include "previewwindow.h"

#include "database.h"
#include "patienttablemodel.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QTableView>
#include <QtConcurrentRun>
#include <QVBoxLayout>

using Page = NavigationController::Page;
using PatientColumn = PatientTableModel::Column;

namespace {
enum class Icon { Clinic, Home, Patients, Search, Add };

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

void configureMeasure(QDoubleSpinBox *field, qreal maximum)
{
    field->setRange(0, maximum);
    field->setDecimals(1);
    field->setSpecialValueText(QStringLiteral("未填写"));
    field->setButtonSymbols(QAbstractSpinBox::NoButtons);
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
    homeNav_->setObjectName(QStringLiteral("homeNav"));
    patientNav_->setObjectName(QStringLiteral("patientNav"));
    homeNav_->setIcon(interfaceIcon(Icon::Home));
    patientNav_->setIcon(interfaceIcon(Icon::Patients));
    for (auto *action : {homeNav_, patientNav_}) {
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
    pages_->addWidget(homePage_);
    pages_->addWidget(patientPage_);
    pages_->addWidget(editorPage_);
    layout->addWidget(appBar);
    layout->addWidget(pages_, 1);

    connect(homeNav_, &QPushButton::clicked, &navigation_, [this] {
        navigation_.selectWorkspacePage(Page::Workbench);
    });
    connect(patientNav_, &QPushButton::clicked, &navigation_, [this] {
        navigation_.selectWorkspacePage(Page::Patients);
    });
    connect(exit, &QPushButton::clicked, &navigation_, &NavigationController::returnToLogin);

    auto *backShortcut = new QShortcut(QKeySequence::Back, page);
    backShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(backShortcut, &QShortcut::activated, &navigation_, &NavigationController::goBack);
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
    for (const QString &name : {QStringLiteral("医生管理"), QStringLiteral("科室管理")}) {
        list->addWidget(divider());
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 14, 0, 14);
        row->addWidget(label(name, "fieldLabel"));
        row->addStretch();
        row->addWidget(label(QStringLiteral("预留"), "muted"));
        list->addLayout(row);
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
    patientTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
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

    patientId_ = new QLineEdit;
    patientId_->setObjectName(QStringLiteral("patientId"));
    patientId_->setMaxLength(32);
    patientName_ = new QLineEdit;
    patientName_->setObjectName(QStringLiteral("patientName"));
    patientName_->setMaxLength(64);
    identityCard_ = new QLineEdit;
    identityCard_->setMaxLength(20);
    sex_ = new QComboBox;
    sex_->addItem(QStringLiteral("请选择"));
    sex_->addItem(QStringLiteral("未知"), 0);
    sex_->addItem(QStringLiteral("男"), 1);
    sex_->addItem(QStringLiteral("女"), 2);
    birthDate_ = new QDateEdit;
    birthDate_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    birthDate_->setCalendarPopup(true);
    birthDate_->setDateRange(QDate(1900, 1, 1), QDate::currentDate());
    birthDate_->setSpecialValueText(QStringLiteral("选择出生日期"));
    mobile_ = new QLineEdit;
    mobile_->setMaxLength(20);
    height_ = new QDoubleSpinBox;
    configureMeasure(height_, 300);
    weight_ = new QDoubleSpinBox;
    configureMeasure(weight_, 1000);

    addField(fields, 0, 0, QStringLiteral("病历编号 *"), patientId_);
    addField(fields, 0, 1, QStringLiteral("姓名 *"), patientName_);
    addField(fields, 1, 0, QStringLiteral("证件号码 *"), identityCard_);
    addField(fields, 1, 1, QStringLiteral("性别 *"), sex_);
    addField(fields, 2, 0, QStringLiteral("出生日期 *"), birthDate_);
    addField(fields, 2, 1, QStringLiteral("手机号码"), mobile_);
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
    editorNotice_ = label(QStringLiteral("预览模式下不会保存表单。"), "muted");
    actions->addWidget(editorNotice_, 1);
    auto *cancel = button(QStringLiteral("取消"));
    cancel->setObjectName(QStringLiteral("cancelEditor"));
    auto *save = button(QStringLiteral("保存"), "primary");
    save->setObjectName(QStringLiteral("savePreview"));
    actions->addWidget(cancel);
    actions->addWidget(save);
    layout->addLayout(actions);
    layout->addStretch();
    connect(back, &QPushButton::clicked, &navigation_, &NavigationController::goBack);
    connect(cancel, &QPushButton::clicked, &navigation_, &NavigationController::goBack);
    connect(save, &QPushButton::clicked, this, [this] {
        editorNotice_->setText(QStringLiteral("未保存。预览模式下无法写入数据库。"));
    });
    return page;
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
    const auto credential = database_.passwordDigestForUser(username, digest);
    if (credential != Database::CredentialLookup::Found) {
        finishLogin(credential == Database::CredentialLookup::UnknownUser
            ? PasswordDigest::Check::Mismatch : PasswordDigest::Check::Unavailable);
        return;
    }

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
        navigation_.enterWorkspace();
        return;
    }

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
        focus = patientId_->isReadOnly() ? patientName_ : patientId_;
        break;
    }

    root_->setCurrentWidget(workspace_);
    focus->setFocus();
}

void PreviewWindow::clearWorkspace()
{
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
    if (patientData_.load(database_.connection(), error) && restoreColumns) {
        configurePatientColumns();
    }
    updatePatientResults(error);
}

void PreviewWindow::updatePatientResults(const QString &error)
{
    patientLoadFailed_ = !error.isEmpty();
    const int count = patients_->rowCount();
    if (patientLoadFailed_) {
        emptyTitle_->setText(QStringLiteral("无法加载患者列表"));
        emptyDescription_->setText(error);
        clearSearch_->setText(QStringLiteral("重试查询"));
        clearSearch_->show();
    } else if (count == 0) {
        const bool filtered = !patients_->filterRegularExpression().pattern().isEmpty();
        emptyTitle_->setText(filtered ? QStringLiteral("未找到患者") : QStringLiteral("暂无患者记录"));
        emptyDescription_->setText(filtered ? QStringLiteral("请检查关键词或查看全部患者。")
                                             : QStringLiteral("当前数据库中没有患者档案。"));
        clearSearch_->setText(QStringLiteral("查看全部"));
        clearSearch_->setVisible(filtered);
    }
    patientResults_->setCurrentIndex(!patientLoadFailed_ && count > 0 ? 0 : 1);
    patientCount_->setText(patientLoadFailed_ ? QStringLiteral("记录数不可用")
                                             : QStringLiteral("%1 条患者记录").arg(count));
    updatePatientActions();
}

void PreviewWindow::configurePatientColumns()
{
    auto *header = patientTable_->horizontalHeader();
    header->setStretchLastSection(false);
    header->resizeSection(PatientColumn::Id, 130);
    header->resizeSection(PatientColumn::Name, 128);
    header->resizeSection(PatientColumn::Sex, 86);
    header->resizeSection(PatientColumn::BirthDate, 144);
    header->resizeSection(PatientColumn::IdentityCard, 196);
    header->resizeSection(PatientColumn::Mobile, 132);
    patientTable_->hideColumn(PatientColumn::Height);
    patientTable_->hideColumn(PatientColumn::Weight);
    header->setStretchLastSection(true);
}

void PreviewWindow::updatePatientActions()
{
    const bool selected = !patientLoadFailed_ && patientTable_->selectionModel()->hasSelection();
    editAction_->setEnabled(selected);
    deleteAction_->setEnabled(selected);
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
    const auto value = [&patient](PatientColumn column) {
        return patient.siblingAtColumn(column).data(Qt::EditRole);
    };
    const bool editing = patient.isValid();
    editorTitle_->setText(editing ? QStringLiteral("编辑患者") : QStringLiteral("新增患者"));
    editorNotice_->setText(QStringLiteral("预览模式下不会保存表单。"));
    patientId_->setText(value(PatientColumn::Id).toString());
    patientId_->setReadOnly(editing);
    patientName_->setText(value(PatientColumn::Name).toString());
    identityCard_->setText(value(PatientColumn::IdentityCard).toString());
    sex_->setCurrentIndex(editing ? sex_->findData(value(PatientColumn::Sex)) : 0);
    birthDate_->setDate(editing ? QDate::fromString(value(PatientColumn::BirthDate).toString(), Qt::ISODate)
                                : birthDate_->minimumDate());
    mobile_->setText(value(PatientColumn::Mobile).toString());
    height_->setValue(value(PatientColumn::Height).toDouble());
    weight_->setValue(value(PatientColumn::Weight).toDouble());
}
