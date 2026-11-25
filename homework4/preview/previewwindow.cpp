#include "previewwindow.h"

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
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

namespace {
enum PatientColumn { Id, Name, Sex, BirthDate, IdentityCard, Mobile, ColumnCount };
enum PatientRole { HeightRole = Qt::UserRole + 1, WeightRole };

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

PreviewWindow::PreviewWindow(QWidget *parent)
    : QWidget(parent)
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

    auto *username = new QLineEdit;
    username->setObjectName(QStringLiteral("username"));
    username->setMaxLength(32);
    password_ = new QLineEdit;
    password_->setObjectName(QStringLiteral("password"));
    password_->setEchoMode(QLineEdit::Password);

    auto *fields = new QGridLayout;
    fields->setContentsMargins(0, 0, 0, 0);
    fields->setVerticalSpacing(13);
    addField(fields, 0, 0, QStringLiteral("用户名"), username);
    addField(fields, 1, 0, QStringLiteral("密码"), password_);
    form->addLayout(fields);
    form->addSpacing(18);

    auto *signIn = button(QStringLiteral("进入预览"), "primary");
    signIn->setObjectName(QStringLiteral("signIn"));
    form->addWidget(signIn);
    form->addSpacing(10);
    form->addWidget(label(QStringLiteral("预览模式无需验证账号。"), "muted"));
    connect(signIn, &QPushButton::clicked, this, [this] { showWorkspacePage(homePage_); });
    connect(username, &QLineEdit::returnPressed, password_, [this] { password_->setFocus(); });
    connect(password_, &QLineEdit::returnPressed, signIn, &QPushButton::click);
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
        action->setIconSize(QSize(16, 16));
        bar->addWidget(action);
    }
    bar->addStretch();
    bar->addWidget(label(QStringLiteral("预览模式"), "muted"));
    bar->addSpacing(10);
    auto *exit = button(QStringLiteral("返回登录"), "quiet");
    exit->setObjectName(QStringLiteral("exitPreview"));
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

    connect(homeNav_, &QPushButton::clicked, this, [this] { showWorkspacePage(homePage_); });
    connect(patientNav_, &QPushButton::clicked, this, [this] { showWorkspacePage(patientPage_); });
    connect(exit, &QPushButton::clicked, this, [this] {
        password_->clear();
        root_->setCurrentWidget(loginPage_);
    });
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
    connect(open, &QPushButton::clicked, this, [this] { showWorkspacePage(patientPage_); });
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

    auto *records = new QStandardItemModel(0, ColumnCount, page);
    records->setHorizontalHeaderLabels({QStringLiteral("病历编号"), QStringLiteral("姓名"),
        QStringLiteral("性别"), QStringLiteral("出生日期"), QStringLiteral("证件号码"),
        QStringLiteral("手机号码")});
    struct DemoPatient {
        QStringList cells;
        double height;
        double weight;
    };
    const DemoPatient demoRows[] = {
        {{QStringLiteral("demo-001"), QStringLiteral("林晓晨"), QStringLiteral("男"),
          QStringLiteral("2000-01-01"), QStringLiteral("000000200001010001"), QStringLiteral("—")},
         172.5, 66.0},
        {{QStringLiteral("demo-002"), QStringLiteral("陈若宁"), QStringLiteral("女"),
          QStringLiteral("1998-05-14"), QStringLiteral("000000199805140002"), QStringLiteral("—")},
         163.0, 54.5},
        {{QStringLiteral("demo-003"), QStringLiteral("罗雨安"), QStringLiteral("未知"),
          QStringLiteral("2003-08-25"), QStringLiteral("000000200308250003"), QStringLiteral("—")},
         0, 0}
    };
    for (const auto &row : demoRows) {
        QList<QStandardItem *> items;
        for (const QString &value : row.cells) {
            items.append(new QStandardItem(value));
        }
        items[Id]->setData(row.height, HeightRole);
        items[Id]->setData(row.weight, WeightRole);
        records->appendRow(items);
    }
    patients_ = new QSortFilterProxyModel(page);
    patients_->setSourceModel(records);
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
    header->resizeSection(Id, 130);
    header->resizeSection(Name, 128);
    header->resizeSection(Sex, 86);
    header->resizeSection(BirthDate, 144);
    header->resizeSection(IdentityCard, 196);
    header->resizeSection(Mobile, 132);
    header->setStretchLastSection(true);

    auto *empty = new QWidget;
    auto *emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setSpacing(8);
    emptyLayout->addStretch();
    emptyLayout->addWidget(label(QStringLiteral("未找到患者"), "sectionTitle"), 0, Qt::AlignHCenter);
    emptyLayout->addWidget(label(QStringLiteral("请检查关键词或查看全部患者。"), "muted"),
                           0, Qt::AlignHCenter);
    auto *clear = button(QStringLiteral("查看全部"));
    clear->setObjectName(QStringLiteral("clearSearch"));
    emptyLayout->addWidget(clear, 0, Qt::AlignHCenter);
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
    footer->addStretch();
    footer->addWidget(label(QStringLiteral("示例数据"), "muted"));
    body->addWidget(foot);
    layout->addWidget(panel, 1);

    connect(runSearch, &QPushButton::clicked, this, &PreviewWindow::filterPatients);
    connect(search_, &QLineEdit::returnPressed, this, &PreviewWindow::filterPatients);
    connect(search_, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.isEmpty()) {
            filterPatients();
        }
    });
    connect(clear, &QPushButton::clicked, this, [this] {
        search_->clear();
        search_->setFocus();
    });
    connect(patientTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PreviewWindow::updatePatientActions);
    connect(editAction_, &QPushButton::clicked, this, [this] {
        showEditor(patientTable_->selectionModel()->selectedRows().value(0));
    });
    connect(patientTable_, &QTableView::doubleClicked, this, &PreviewWindow::showEditor);
    connect(add, &QPushButton::clicked, this, [this] { showEditor(QModelIndex()); });
    connect(deleteAction_, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, QStringLiteral("删除患者"),
            QStringLiteral("预览模式下无法删除患者记录。"));
    });
    filterPatients();
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
    sex_->addItems({QStringLiteral("请选择"), QStringLiteral("未知"), QStringLiteral("男"), QStringLiteral("女")});
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
    connect(back, &QPushButton::clicked, this, [this] { showWorkspacePage(patientPage_); });
    connect(cancel, &QPushButton::clicked, back, &QPushButton::click);
    connect(save, &QPushButton::clicked, this, [this] {
        editorNotice_->setText(QStringLiteral("未保存。预览模式下无法写入数据库。"));
    });
    return page;
}

void PreviewWindow::showWorkspacePage(QWidget *page)
{
    root_->setCurrentWidget(workspace_);
    pages_->setCurrentWidget(page);
    homeNav_->setChecked(page == homePage_);
    patientNav_->setChecked(page != homePage_);
    if (page == homePage_) {
        homeNav_->setFocus();
    } else if (page == patientPage_) {
        search_->setFocus();
    }
}

void PreviewWindow::filterPatients()
{
    patientTable_->clearSelection();
    patients_->setFilterFixedString(search_->text().trimmed());
    const int count = patients_->rowCount();
    patientResults_->setCurrentIndex(count > 0 ? 0 : 1);
    patientCount_->setText(QStringLiteral("%1 条患者记录").arg(count));
    updatePatientActions();
}

void PreviewWindow::updatePatientActions()
{
    const bool selected = patientTable_->selectionModel()->hasSelection();
    editAction_->setEnabled(selected);
    deleteAction_->setEnabled(selected);
}

void PreviewWindow::showEditor(const QModelIndex &patient)
{
    const auto value = [&patient](int column) {
        return patient.siblingAtColumn(column).data().toString();
    };
    const bool editing = patient.isValid();
    editorTitle_->setText(editing ? QStringLiteral("编辑患者") : QStringLiteral("新增患者"));
    editorNotice_->setText(QStringLiteral("预览模式下不会保存表单。"));
    patientId_->setText(value(Id));
    patientId_->setReadOnly(editing);
    patientName_->setText(value(Name));
    identityCard_->setText(value(IdentityCard));
    sex_->setCurrentIndex(editing ? sex_->findText(value(Sex)) : 0);
    birthDate_->setDate(editing ? QDate::fromString(value(BirthDate), Qt::ISODate)
                                : birthDate_->minimumDate());
    mobile_->setText(value(Mobile) == QStringLiteral("—") ? QString() : value(Mobile));
    height_->setValue(patient.siblingAtColumn(Id).data(HeightRole).toDouble());
    weight_->setValue(patient.siblingAtColumn(Id).data(WeightRole).toDouble());
    showWorkspacePage(editorPage_);
    (editing ? patientName_ : patientId_)->setFocus();
}
