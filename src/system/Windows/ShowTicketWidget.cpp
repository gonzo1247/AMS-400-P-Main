#include "ShowTicketWidget.h"

#include "TicketAgeTextDelegate.h"

ShowTicketWidget::ShowTicketWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ShowTicketWidgetClass()), _ticketModel(nullptr), _ticketMgr(nullptr), _ticketDetailWidget(nullptr)
{
	ui->setupUi(this);
    LoadTableData();

    {
        auto updateFilter = [this]()
        {
            QString filter;

            if (!ui->le_search->text().trimmed().isEmpty())
                filter += ui->le_search->text().trimmed() + " ";

            filter = filter.trimmed();

            _ticketModel->setFilterText(filter);
        };

        connect(ui->le_search, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });

        connect(ui->pb_reset, &QPushButton::clicked, this,
                [this]()
                {
                    ui->le_search->clear();

                    _ticketModel->setFilterText(QString());
                });
    }

    connect(ui->tv_showTickets, &QTableView::doubleClicked, this,
            [this](const QModelIndex& idx)
            {
                auto id = _ticketModel->ticketIdForIndex(idx);

                if (!id)
                    return;

                if (!_ticketDetailWidget)
                {
                    _ticketDetailWidget = new ShowTicketDetailWidget(this);
                    _ticketDetailWidget->setAttribute(Qt::WA_DeleteOnClose);
                    _ticketDetailWidget->setWindowFlag(Qt::Window, true);
                }



                _ticketDetailWidget->LoadAndFillData(*id);
                _ticketDetailWidget->show();
            });

    auto* timer = new QTimer(this);
    timer->setInterval(60 * 1000);  // 1 minute
    connect(timer, &QTimer::timeout, _ticketModel, &TicketTableModel::refreshOpenDurations);
    timer->start();

    connect(ui->pb_manualRefresh, &QPushButton::clicked, this, &ShowTicketWidget::onPushManualRefreshButton);

    auto* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(5 * 60 * 1000);  // 5 minutes
    connect(refreshTimer, &QTimer::timeout, this, &ShowTicketWidget::onPushManualRefreshButton);
    refreshTimer->start();

    _ticketDetailWidget = new ShowTicketDetailWidget(nullptr);
    _ticketDetailWidget->setWindowFlag(Qt::Window, true);
    connect(_ticketDetailWidget, &ShowTicketDetailWidget::TicketClosed, this, &ShowTicketWidget::onPushManualRefreshButton);
}

ShowTicketWidget::~ShowTicketWidget()
{
	delete ui;
}

void ShowTicketWidget::LoadTableData()
{
    if (!_ticketModel)
	{
		_ticketModel = new TicketTableModel(this);
        ui->tv_showTickets->setItemDelegateForColumn(13, new TicketAgeTextDelegate(ui->tv_showTickets));
		ui->tv_showTickets->setModel(_ticketModel);
		ui->tv_showTickets->setSortingEnabled(true);
    }

	if (!_ticketMgr)
        _ticketMgr = std::make_unique<ShowTicketManager>();

    auto task = [this]()
    {
        _ticketMgr->LoadTableTicketData();
        auto localMap = _ticketMgr->GetTableTicketVector();
      //  _callerTableData = localMap;

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                // Ensure model exists once
                if (!_ticketModel)
                {
                    _ticketModel = new TicketTableModel(this);
                    ui->tv_showTickets->setItemDelegateForColumn(13, new TicketAgeTextDelegate(ui->tv_showTickets));
                    ui->tv_showTickets->setModel(_ticketModel);
                    ui->tv_showTickets->setSortingEnabled(true);
                }

                _ticketModel->setRows(localMap);
                SetupTable();
                _refresh.store(false);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketWidget::SetupTable()
{
    auto* tv = ui->tv_showTickets;
    auto* header = tv->horizontalHeader();

    tv->setWordWrap(false);
    tv->setTextElideMode(Qt::ElideRight);

    header->setHighlightSections(false);
    header->setStretchLastSection(false);

    header->setSectionResizeMode(QHeaderView::Interactive);

    // Fixed / stable columns
    header->setSectionResizeMode(0, QHeaderView::Fixed);   // ID
    header->setSectionResizeMode(5, QHeaderView::Fixed);   // Priority
    header->setSectionResizeMode(6, QHeaderView::Fixed);   // Status
    header->setSectionResizeMode(7, QHeaderView::Fixed);   // Created
    header->setSectionResizeMode(8, QHeaderView::Fixed);   // Updated
    header->setSectionResizeMode(10, QHeaderView::Fixed);  // Comments
    header->setSectionResizeMode(11, QHeaderView::Fixed);  // History
    header->setSectionResizeMode(13, QHeaderView::Fixed);  // Open For

    // Content sized columns
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);   // Area
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);   // Reporter
    header->setSectionResizeMode(12, QHeaderView::ResizeToContents);  // Cost Unit

    // Main text columns
    header->setSectionResizeMode(1, QHeaderView::Stretch);      // Title
    header->setSectionResizeMode(4, QHeaderView::Stretch);      // Machine
    header->setSectionResizeMode(9, QHeaderView::Interactive);  // Assigned

    tv->setColumnWidth(0, 90);
    tv->setColumnWidth(5, 120);
    tv->setColumnWidth(6, 140);
    tv->setColumnWidth(7, 155);
    tv->setColumnWidth(8, 155);
    tv->setColumnWidth(10, 70);
    tv->setColumnWidth(11, 70);
    tv->setColumnWidth(13, 170);

    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Optional: default sort by ID desc (newest on top)
    tv->sortByColumn(0, Qt::DescendingOrder);

    ui->tv_showTickets->hideColumn(2);   // Hide Area column by default
    ui->tv_showTickets->hideColumn(10);  // Hide Comments column by default
    ui->tv_showTickets->hideColumn(11);  // Hide History column by default
}

void ShowTicketWidget::onPushManualRefreshButton()
{
    // Atomically claim the refresh slot
    if (_refresh.exchange(true))
        return;  // refresh already running

    LoadTableData();
}

