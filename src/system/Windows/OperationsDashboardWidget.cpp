#include "OperationsDashboardWidget.h"

#include <qtableview.h>

#include <QEasingCurve>
#include <QScrollBar>
#include <QTimer>

#include "ContractorVisitLeftOverlayDelegate.h"
#include "TicketAgeTextDelegate.h"
#include "TicketColorDelegate.h"
#include "Util.h"
#include "WordWrapItemDelegate.h"
#include "pch.h"

OperationsDashboardWidget::OperationsDashboardWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::OperationsDashboardWidgetClass()), _ticketModel(nullptr), _contractVisitModel(nullptr)
{
    ui->setupUi(this);

    ui->tv_tickets->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tv_tickets->setFocusPolicy(Qt::NoFocus);
    ui->tv_tickets->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    EnsureTicketUi();

    _ticketMgr = std::make_unique<ShowTicketManager>();
    _contractVisitMgr = std::make_unique<ContractorVisitManager>();

    _lastSyncDb = "1970-01-01 00:00:00";

    LoadTicketsInitial();
    StartAutoRefresh();

    // Contractor Visits
    LoadContractorInitial();

    // Connections
    connect(ui->pB_ExitFullscreen, &QPushButton::clicked, this, &OperationsDashboardWidget::onPushExitFullscreen);
}

OperationsDashboardWidget::~OperationsDashboardWidget() { delete ui; }

void OperationsDashboardWidget::setTextPointSize(uint8_t size)
{
    const int pt = std::clamp(static_cast<int>(size), 6, 24);

    auto apply = [pt](QTableView* view)
    {
        if (view == nullptr)
            return;

        QFont f = view->font();
        f.setPointSize(pt);

        view->setFont(f);
        if (view->horizontalHeader() != nullptr)
            view->horizontalHeader()->setFont(f);

        if (view->verticalHeader() != nullptr)
            view->verticalHeader()->setFont(f);

        view->viewport()->update();
    };

    apply(ui->tv_tickets);
    apply(ui->tv_contractorVisits);
}

void OperationsDashboardWidget::increaseTextPointSize()
{
    setTextPointSize(static_cast<uint8_t>(GetTextPointSize() + 1));
    _ticketsScroller->BeginTicketTableUpdate();
    ApplyTicketViewLayout(true);
    _ticketsScroller->EndTicketTableUpdate();
    EnsureTicketUi();
}

void OperationsDashboardWidget::decreaseTextPointSize()
{
    const uint8_t current = GetTextPointSize();
    setTextPointSize(static_cast<uint8_t>(current > 0 ? current - 1 : 0));
    _ticketsScroller->BeginTicketTableUpdate();
    ApplyTicketViewLayout(true);
    _ticketsScroller->EndTicketTableUpdate();
    EnsureTicketUi();
}

uint8_t OperationsDashboardWidget::GetTextPointSize()
{
    QTableView* view = ui->tv_tickets;
    if (view == nullptr)
        return 10;

    QFont f = view->font();

    int pt = f.pointSize();
    if (pt < 0)
        pt = QFontInfo(f).pointSize();

    pt = std::clamp(pt, 6, 24);
    return static_cast<uint8_t>(pt);
}

void OperationsDashboardWidget::EnsureTicketUi()
{
    if (!_ticketModel)
    {
        _ticketModel = new TicketOperationDashboardModel(this);
        ui->tv_tickets->setModel(_ticketModel);

        // Register autoscroller
        _ticketsScroller = new TableViewAutoScroller(ui->tv_tickets, this);
        _ticketsScroller->HookModelForScrollPreserve();
        _ticketsScroller->Start();

        // Delegate
        if (!_ticketDelegate)
            _ticketDelegate = new TicketColorAndWordWarpDelegate(ui->tv_tickets);

        ui->tv_tickets->setItemDelegate(_ticketDelegate);

        ui->tv_tickets->setWordWrap(true);
        ui->tv_tickets->setTextElideMode(Qt::ElideNone);

        // Hide unused
        ui->tv_tickets->hideColumn(10);  // Comments
        ui->tv_tickets->hideColumn(11);  // History
    }

    if (!_contractVisitModel)
    {
        _contractVisitModel = new ContractorVisitModel(ContractorVisitModel::ViewMode::Dashboard, this);
        ui->tv_contractorVisits->setModel(_contractVisitModel);

        // register autoscroller
        _contractVisitsScroller = new TableViewAutoScroller(ui->tv_contractorVisits, this);

        auto* del = new ContractorVisitLeftOverlayDelegate(ui->tv_contractorVisits);
        ui->tv_contractorVisits->setItemDelegate(del);

        ui->tv_contractorVisits->setSelectionMode(QAbstractItemView::NoSelection);
        ui->tv_contractorVisits->setFocusPolicy(Qt::NoFocus);
        ui->tv_contractorVisits->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
}

void OperationsDashboardWidget::LoadTicketsInitial()
{
    if (_refreshActive.exchange(true))
        return;

    auto task = [this]()
    {
        _ticketMgr->LoadTableTicketData();
        auto rows = _ticketMgr->GetTableTicketVectorNoCopy();

        const std::string newSync = Util::CurrentDateTimeStringStd();

        QMetaObject::invokeMethod(
            this,
            [this, rows = std::move(rows), newSync]()
            {
                _ticketModel->setRows(rows);

                ui->tv_tickets->setSortingEnabled(true);
                ui->tv_tickets->sortByColumn(0, Qt::AscendingOrder);

                _lastSyncDb = newSync;
                _ticketsScroller->BeginTicketTableUpdate();
                ApplyTicketViewLayout(true);
                _ticketsScroller->EndTicketTableUpdate();
                EnsureTicketUi();
                _refreshActive.store(false);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void OperationsDashboardWidget::LoadTicketsDelta()
{
    if (_refreshActive.exchange(true))
        return;

    const std::string since = _lastSyncDb;

    auto task = [this, since]()
    {
        auto delta = _ticketMgr->LoadTableTicketDelta(since);

        QMetaObject::invokeMethod(
            this,
            [this, delta = std::move(delta)]() mutable
            {
                _ticketModel->ApplyDelta(delta.upserts, delta.removedIds);
                _lastSyncDb = Util::FormatDateTimeStd(delta.newSyncPoint);
                _ticketsScroller->BeginTicketTableUpdate();
                ApplyTicketViewLayout(true);
                _ticketsScroller->EndTicketTableUpdate();
                EnsureTicketUi();

                _refreshActive.store(false);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void OperationsDashboardWidget::StartAutoRefresh()
{
    if (!_refreshTimer)
    {
        _refreshTimer = new QTimer(this);
        connect(_refreshTimer, &QTimer::timeout, this, [this]() { LoadTicketsInitial(); });
    }

    if (!_openForTimer)
    {
        _openForTimer = new QTimer(this);
        connect(_openForTimer, &QTimer::timeout, this,
                [this]()
                {
                    if (_ticketModel)
                        _ticketModel->refreshOpenDurations();
                });
    }

    _refreshTimer->start(30 * 1000);  // DB delta refresh
    _openForTimer->start(60 * 1000);  // update "Open For" column + delegate colors
}

void OperationsDashboardWidget::ApplyTicketViewLayout(bool tvMode)
{
    auto* v = ui->tv_tickets;
    if (!v || !v->model())
    {
        return;
    }

    v->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v->setWordWrap(true);
    v->setTextElideMode(Qt::ElideNone);

    v->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    v->verticalHeader()->setMinimumSectionSize(32);

    auto* hh = v->horizontalHeader();
    hh->setStretchLastSection(false);
    hh->setMinimumSectionSize(40);
    hh->setSectionResizeMode(QHeaderView::Fixed);

    auto clamp = [](int value, int lo, int hi) { return std::max(lo, std::min(value, hi)); };
    auto isVisible = [&](int col) { return !v->isColumnHidden(col); };

    struct Rule
    {
        int col;
        int minPx;
        int maxPx;
        bool useContentsHint;
        int preferredPx;
    };

    const std::vector<Rule> fixedRules = {
        {0, 0, 110, true, 90},      // ID
        {3, 150, 220, true, 0},     // Reporter
        {4, 150, 280, true, 0},     // Machine
        {5, 95, 130, false, 110},   // Priority
        {6, 100, 150, false, 120},  // Status
        {7, 130, 190, false, 160},  // Created
        {9, 140, 220, true, 0},     // Assigned
        {13, 110, 170, false, 140}  // Open For
    };

    for (const auto& r : fixedRules)
    {
        if (!isVisible(r.col))
        {
            continue;
        }

        int w = r.preferredPx;

        if (r.useContentsHint)
        {
            v->resizeColumnToContents(r.col);
            w = v->columnWidth(r.col) + 18;
        }

        w = clamp(w, r.minPx, r.maxPx);
        hh->resizeSection(r.col, w);
    }

    v->hideColumn(10);
    v->hideColumn(11);
    v->hideColumn(2);

    if (tvMode)
    {
        v->setColumnHidden(8, true);
        v->setColumnHidden(12, true);
    }
    else
    {
        v->setColumnHidden(4, false);
        v->setColumnHidden(8, false);
        v->setColumnHidden(9, false);
        v->setColumnHidden(12, false);
    }

    const int titleCol = 1;
    const int commentCol = 14;

    if (isVisible(commentCol))
    {
        const int commentW = tvMode ? 260 : 220;
        hh->setSectionResizeMode(commentCol, QHeaderView::Fixed);
        hh->resizeSection(commentCol, commentW);
    }

    if (isVisible(titleCol))
    {
        hh->setSectionResizeMode(titleCol, QHeaderView::Stretch);
    }

    v->resizeRowsToContents();
}

void OperationsDashboardWidget::LoadContractorInitial()
{
    if (_refreshActiveContractor.exchange(true))
        return;

    auto task = [this]()
    {
        _contractVisitMgr->LoadContractorVisitData();
        auto rows = _contractVisitMgr->GetContractorVisits();

        const std::string newSync = Util::CurrentDateTimeStringStd();

        QMetaObject::invokeMethod(
            this,
            [this, rows = std::move(rows), newSync]()
            {
                EnsureTicketUi();

                _contractVisitModel->setRows(rows);

                // _lastSyncDb = newSync;
                ApplyContractorVisitViewLayout(true);

                _refreshActiveContractor.store(false);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void OperationsDashboardWidget::ApplyContractorVisitViewLayout(bool tvMode)
{
    using Column = ContractorVisitModel::Column;
    auto* v = ui->tv_contractorVisits;

    v->setWordWrap(false);
    v->horizontalHeader()->setStretchLastSection(false);
    v->horizontalHeader()->setMinimumSectionSize(40);

    // Default: hide stuff we never want
    v->hideColumn(Column::COL_COMPANY_EXTERNAL_ID);
    v->hideColumn(Column::COL_CREATED_BY_USER_ID);
    v->hideColumn(Column::COL_UPDATED_AT);

    if (tvMode)
    {
        // Hide detail columns on TV
        v->setColumnHidden(Column::COL_COMPANY_EXTERNAL_ID, true);
        v->setColumnHidden(Column::COL_NOTE, true);
        v->setColumnHidden(Column::COL_REACHABLE_NOTE, true);
        v->setColumnHidden(Column::COL_CREATED_AT, true);

        // Resize important columns
        v->horizontalHeader()->setSectionResizeMode(Column::COL_ID, QHeaderView::ResizeToContents);   // ID
        v->horizontalHeader()->setSectionResizeMode(Column::COL_COMPANY_NAME, QHeaderView::Stretch);  // Company
        v->horizontalHeader()->setSectionResizeMode(Column::COL_CONTACT_PERSON,
                                                    QHeaderView::ResizeToContents);                           // Contact
        v->horizontalHeader()->setSectionResizeMode(Column::COL_WORKER_NAME, QHeaderView::ResizeToContents);  // Worker
        v->horizontalHeader()->setSectionResizeMode(Column::COL_ACTIVITY, QHeaderView::Stretch);             // Activity
        v->horizontalHeader()->setSectionResizeMode(Column::COL_LOCATION, QHeaderView::ResizeToContents);    // Location
        v->horizontalHeader()->setSectionResizeMode(Column::COL_ARRIVAL_AT, QHeaderView::ResizeToContents);  // Arrival
        v->horizontalHeader()->setSectionResizeMode(Column::COL_DEPARTURE_AT,
                                                    QHeaderView::ResizeToContents);                      // Departure
        v->horizontalHeader()->setSectionResizeMode(Column::COL_STATUS, QHeaderView::ResizeToContents);  // Status
        v->horizontalHeader()->setSectionResizeMode(Column::COL_REACHABLE_PHONE,
                                                    QHeaderView::ResizeToContents);  // Phone

        // Optional fixed widths for calmer TV layout
        v->setColumnWidth(Column::COL_CONTACT_PERSON, 160);
        v->setColumnWidth(Column::COL_WORKER_NAME, 200);
        v->setColumnWidth(Column::COL_LOCATION, 140);
        v->setColumnWidth(Column::COL_STATUS, 110);
        v->setColumnWidth(Column::COL_ARRIVAL_AT, 160);
        v->setColumnWidth(Column::COL_DEPARTURE_AT, 160);
    }
    else
    {
        // Desktop mode: show more information
        v->setColumnHidden(Column::COL_COMPANY_EXTERNAL_ID, false);
        v->setColumnHidden(Column::COL_NOTE, false);
        v->setColumnHidden(Column::COL_REACHABLE_NOTE, false);
        v->setColumnHidden(Column::COL_CREATED_AT, false);

        v->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        v->horizontalHeader()->setSectionResizeMode(Column::COL_COMPANY_NAME, QHeaderView::Stretch);
        v->horizontalHeader()->setSectionResizeMode(Column::COL_ACTIVITY, QHeaderView::Stretch);
    }
}

void OperationsDashboardWidget::onPushExitFullscreen()
{
    if (this->window() == nullptr)
        return;

    if (this->window()->isFullScreen())
    {
        this->window()->showNormal();
        this->window()->setWindowState(Qt::WindowNoState);
    }
}
