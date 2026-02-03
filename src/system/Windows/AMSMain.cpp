#include "AMSMain.h"

#include <QCloseEvent>
#include <QWindow>

#include "ContractorWorkerData.h"
#include "CostUnitDataHandler.h"
#include "Databases.h"
#include "GlobalSignals.h"
#include "MachineDataHandler.h"
#include "MessageBoxHelper.h"
#include "NetworkShareConnector.h"
#include "OperationsDashboardControlWidget.h"
#include "RBACAccess.h"
#include "RBACVisibilityHelper.h"
#include "ShutdownManager.h"
#include "Version.h"
#include "ContractVisit.h"

AMSMain::AMSMain(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::AMSMainClass()),
      _callerMgrWidget(nullptr),
      _callerMgrWidgetExternal(nullptr),
      _createTicketWidgetExternal(nullptr),
      _createTicketWidget(nullptr),
      _employeeMgrWidget(nullptr),
      _showTicketWidget(nullptr),
      _operationsDashboardWidget(nullptr),
      _userMgr(nullptr),
      _assetManagerWidget(nullptr),
      _operationsDashboardControlWidget(nullptr),
      _operationsDashboardWidgetExternal(nullptr)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);

    /*
        auto imsdia = IMSDatabase::GetDiagnostics();

        LOG_DEBUG(
            "IMS DiagnosticsSnapshot:"
            " syncPoolSize={} asyncPoolSize={} replicaSyncPoolSize={} replicaAsyncPoolSize={}"
            " syncAvailable={} asyncAvailable={} replicaSyncAvailable={} replicaAsyncAvailable={}"
            " queuedJobs={} registeredStatements={}",
            imsdia.syncPoolSize, imsdia.asyncPoolSize, imsdia.replicaSyncPoolSize, imsdia.replicaAsyncPoolSize,
            imsdia.syncAvailable, imsdia.asyncAvailable, imsdia.replicaSyncAvailable, imsdia.replicaAsyncAvailable,
            imsdia.queuedJobs, imsdia.registeredStatements);

        auto amsdia = AMSDatabase::GetDiagnostics();
        LOG_DEBUG(
            "AMS DiagnosticsSnapshot:"
            " syncPoolSize={} asyncPoolSize={} replicaSyncPoolSize={} replicaAsyncPoolSize={}"
            " syncAvailable={} asyncAvailable={} replicaSyncAvailable={} replicaAsyncAvailable={}"
            " queuedJobs={} registeredStatements={}",
            amsdia.syncPoolSize, amsdia.asyncPoolSize, amsdia.replicaSyncPoolSize, amsdia.replicaAsyncPoolSize,
            amsdia.syncAvailable, amsdia.asyncAvailable, amsdia.replicaSyncAvailable, amsdia.replicaAsyncAvailable,
            amsdia.queuedJobs, amsdia.registeredStatements);*/

    CreateConnections();

    _statusLabel = new QLabel(this);
    _statusLabel->clear();
    ui->statusBar->addWidget(_statusLabel);

    ui->le_userName->setFocus();
    ApplyRBACVisibility();

    ui->lb_versionInformation->setText(QString::fromStdString(
        std::string(VERSION_INFO) + " - " + std::string(VERSION_NUMBER) + " - " + std::string(VERSION_DATE)));
    ui->lb_developerInformation->setText(QString::fromStdString(std::string(BUILD_STATUS)));

    EnsureStatusTimer();
}

AMSMain::~AMSMain()
{
    delete ui;
    delete _contractorWorkerDataWidget;
}

void AMSMain::CreateConnections()
{
    connect(ui->pb_login, &QPushButton::clicked, this, &AMSMain::onPushLogin);
    connect(ui->le_password, &QLineEdit::returnPressed, this, &AMSMain::onPushLogin);
    connect(ui->pb_logout, &QPushButton::clicked, this, &AMSMain::onPushLogout);
    connect(ui->pb_p1_callerMgr, &QPushButton::clicked, this, &AMSMain::onPushCallerManager);
    connect(ui->pb_createTicket, &QPushButton::clicked, this, &AMSMain::onPushCreateTicket);
    connect(ui->pb_showTicket, &QPushButton::clicked, this, &AMSMain::onPushShowTicket);
    connect(ui->pb_p1_employeeMgr, &QPushButton::clicked, this, &AMSMain::onPushEmployeeManager);
    connect(ui->pb_backToMainpage, &QPushButton::clicked, this, &AMSMain::DisplayMainPage);
    connect(ui->pb_showTV, &QPushButton::clicked, this, &AMSMain::onPushShowTV);
    connect(ui->pb_assetManager, &QPushButton::clicked, this, &AMSMain::onPushShowAssetManager);
    connect(ui->pb_contractorWorker, &QPushButton::clicked, this, &AMSMain::onPushContractorWorkerOpen);
    connect(ui->pb_p1_contractorVisit, &QPushButton::clicked, this, &AMSMain::onPushContractorVisit);

    {
        connect(GlobalSignals::instance(), &GlobalSignals::LogoutSignal, this, &AMSMain::LogoutAndReset);
        connect(GlobalSignals::instance(), &GlobalSignals::SendStatusMessage, this, &AMSMain::ReceiveStatusMessage);
        connect(GlobalSignals::instance(), &GlobalSignals::CreateTicketBackToMainPage, this, &AMSMain::DisplayMainPage);
    }

    auto task = [this]() { CostUnitDataHandler::instance().Initialize(); };

    Util::RunInThread(task, this);

    MachineDataHandler::instance().Initialize();
}

void AMSMain::LogoutAndReset()
{
    GetUser().UserLogout();
    ui->stackedWidget->setCurrentIndex(PAGE_LOGIN);
    ui->le_userName->setFocus();
}

void AMSMain::ApplyRBACVisibility()
{
    ChangeVisibilityAndTooltip(Permission::RBAC_OPEN_EMPLOYEE_MANAGER, ui->pb_p1_employeeMgr);
    ChangeVisibilityAndTooltip(Permission::RBAC_OPEN_CALLER_MANAGER, ui->pb_p1_callerMgr);
    ChangeVisibilityAndTooltip(Permission::RBAC_CREATE_TICKET, ui->pb_createTicket);
    ChangeVisibilityAndTooltip(Permission::RBAC_SHOW_TICKETS, ui->pb_showTicket);
    ChangeVisibilityAndTooltip(Permission::RBAC_OPEN_TV, ui->pb_showTV);
    ChangeVisibilityAndTooltip(Permission::RBAC_OPEN_MACHINE_AND_LINES, ui->pb_assetManager);
    ChangeVisibilityAndTooltip(Permission::RBAC_SEARCH_OPEN_TICKETS, ui->pb_ticketSearch);
}

void AMSMain::onPushLogin()
{
    if (!_userMgr)
        _userMgr = std::make_unique<User>();

    DebugLogin();

    if (_userMgr->UserLoginDB(ui->le_userName->text().toStdString(), ui->le_password->text().toStdString()))
    {
        ui->stackedWidget->setCurrentIndex(PAGE_MAIN);
        ui->le_password->clear();
        ui->le_userName->clear();

        QString firstname = QString::fromStdString(GetUser().GetUserData().userFirstName);
        QString lastname = QString::fromStdString(GetUser().GetUserData().userLastName);

        ui->lb_Firstname_Lastname->setText(firstname + " " + lastname);

        ui->lb_welcome->setText(Util::BuildWelcomeText());
        ApplyRBACVisibility();
    }
    else
        MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_PASSWORD_OR_USER_WRONG, true);
}

void AMSMain::onPushLogout() { emit GlobalSignals::instance() -> LogoutSignal(); }

void AMSMain::DisplayExternalWidget(QWidget* widget, bool deletePreviousWidget)
{
    if (!widget)
        return;

    // Remove old widgets from layout
    QLayoutItem* item = nullptr;
    while ((item = ui->vl_page_external_widget->takeAt(0)) != nullptr)
    {
        if (QWidget* w = item->widget())
        {
            if (deletePreviousWidget)
                w->deleteLater();  // delete old widget
            else
                w->setParent(nullptr);  // keep old widget
        }
        delete item;
    }

    ui->vl_page_external_widget->addWidget(widget);

    ui->stackedWidget->setCurrentIndex(PAGE_EXTERNAL_WIDGET);
}

void AMSMain::DisplayMainPage() { ui->stackedWidget->setCurrentIndex(PAGE_MAIN); }

void AMSMain::onPushCallerManager()
{
    const bool shiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

    if (shiftPressed)
    {
        // External window
        if (_callerMgrWidgetExternal != nullptr)
        {
            _callerMgrWidgetExternal->show();
            _callerMgrWidgetExternal->raise();
            _callerMgrWidgetExternal->activateWindow();
            _callerMgrWidgetExternal->setExternal();
            return;
        }

        _callerMgrWidgetExternal = new CallerManagerWidget(this);
        _callerMgrWidgetExternal->setWindowFlag(Qt::Window);

        connect(_callerMgrWidgetExternal, &QObject::destroyed, this, [this]() { _callerMgrWidgetExternal = nullptr; });

        _callerMgrWidgetExternal->show();
        _callerMgrWidgetExternal->setExternal();
        return;
    }

    // Internal instance in stacked page
    if (_callerMgrWidget == nullptr)
    {
        _callerMgrWidget = new CallerManagerWidget(this);
    }

    DisplayExternalWidget(_callerMgrWidget);
}

void AMSMain::onPushCreateTicket()
{
    const bool shiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

    if (shiftPressed)
    {
        // External window
        if (_createTicketWidgetExternal)
        {
            _createTicketWidgetExternal->show();
            _createTicketWidgetExternal->raise();
            _createTicketWidgetExternal->activateWindow();
            _createTicketWidgetExternal->SetExternal();
            return;
        }

        // Top-level window, no QWidget parent
        _createTicketWidgetExternal = new CreateTicketWidget();
        _createTicketWidgetExternal->setWindowFlag(Qt::Window);
        _createTicketWidgetExternal->setAttribute(Qt::WA_DeleteOnClose, true);  // Auto delete

        _createTicketWidgetExternal->show();
        _createTicketWidgetExternal->SetExternal();
        return;
    }

    // Internal instance in stacked page
    if (_createTicketWidget == nullptr)
    {
        _createTicketWidget = new CreateTicketWidget(this);
    }

    DisplayExternalWidget(_createTicketWidget);
}

void AMSMain::onPushShowTicket()
{
    if (!_showTicketWidget)
        _showTicketWidget = new ShowTicketWidget(this);

    DisplayExternalWidget(_showTicketWidget);
}

void AMSMain::onPushEmployeeManager()
{
    const bool shiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

    if (shiftPressed)
    {
        // External window
        if (_employeeMgrWidgetExternal != nullptr)
        {
            _employeeMgrWidgetExternal->show();
            _employeeMgrWidgetExternal->raise();
            _employeeMgrWidgetExternal->activateWindow();
            //_employeeMgrWidgetExternal->setExternal();
            return;
        }

        _employeeMgrWidgetExternal = new EmployeeManagerWidget(this);
        _employeeMgrWidgetExternal->setWindowFlag(Qt::Window);

        connect(_employeeMgrWidgetExternal, &QObject::destroyed, this,
                [this]() { _employeeMgrWidgetExternal = nullptr; });

        _employeeMgrWidgetExternal->show();
        //_employeeMgrWidgetExternal->setExternal();
        return;
    }

    // Internal instance in stacked page
    if (_employeeMgrWidget == nullptr)
    {
        _employeeMgrWidget = new EmployeeManagerWidget(this);
    }

    DisplayExternalWidget(_employeeMgrWidget);
}

void AMSMain::onCallerReturnToMainPage() { DisplayMainPage(); }

void AMSMain::onCreateTicketBack() { DisplayMainPage(); }

void AMSMain::onPushShowTV()
{
    const bool shiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

    if (shiftPressed)
    {
        _operationsDashboardWidgetExternal = new OperationsDashboardWidget(this);
        _operationsDashboardWidgetExternal->setWindowFlag(Qt::Window);
        _operationsDashboardWidgetExternal->setAttribute(Qt::WA_DeleteOnClose);

        connect(_operationsDashboardWidgetExternal, &QObject::destroyed, this,
                [this]()
                {
                    _operationsDashboardWidgetExternal = nullptr;
                    DisplayMainPage();
                });

        _operationsDashboardWidgetExternal->show();
        _operationsDashboardWidgetExternal->raise();
        _operationsDashboardWidgetExternal->activateWindow();

        _operationsDashboardControlWidget =
            new OperationsDashboardControlWidget(_operationsDashboardWidgetExternal, this);
        DisplayExternalWidget(_operationsDashboardControlWidget);

        return;
    }

    if (!_operationsDashboardWidget)
        _operationsDashboardWidget = new OperationsDashboardWidget(this);

    DisplayExternalWidget(_operationsDashboardWidget);
}

void AMSMain::onPushShowAssetManager()
{
    if (!_assetManagerWidget)
    {
        _assetManagerWidget = new AssetManager();
        _assetManagerWidget->setWindowFlags(Qt::Window);
        _assetManagerWidget->setAttribute(Qt::WA_DeleteOnClose);

        connect(_assetManagerWidget, &QObject::destroyed, this, [this]() { _assetManagerWidget = nullptr; });
    }

    _assetManagerWidget->setWindowTitle(tr("Assets / Machines & Lines"));
    _assetManagerWidget->resize(1000, 700);
    _assetManagerWidget->show();
    _assetManagerWidget->raise();
    _assetManagerWidget->activateWindow();
}

void AMSMain::onPushContractorWorkerOpen()
{
    if (!_contractorWorkerDataWidget)
        _contractorWorkerDataWidget = new ContractorWorkerData(this);

    DisplayExternalWidget(_contractorWorkerDataWidget);
}

void AMSMain::onPushContractorVisit()
{
    if (!_contractorVisitWidget)
        _contractorVisitWidget = new ContractVisit(this);

    _contractorVisitWidget->LoadTableData();

    DisplayExternalWidget(_contractorVisitWidget);
}

void AMSMain::ReceiveStatusMessage(const std::string& message, StatusMessageQueue queue, const QColor& color)
{
    const QString qText = QString::fromStdString(message);

    if (queue == StatusMessageQueue::INSTANT)
    {
        if (_statusTimer && _statusTimer->isActive())
        {
            _statusTimer->stop();
        }

        ShowInstantStatus(qText, color);
        return;
    }

    // QUEUE
    _statusQueue.push(StatusItem{qText, color});

    if (_statusTimer && !_statusTimer->isActive())
    {
        ShowNextQueuedStatus();
        _statusTimer->start();
    }
}

void AMSMain::EnsureStatusTimer()
{
    if (_statusTimer)
    {
        return;
    }

    _statusTimer = new QTimer(this);
    _statusTimer->setInterval(_statusIntervalMs);

    connect(_statusTimer, &QTimer::timeout, this, &AMSMain::ShowNextQueuedStatus);
}

bool AMSMain::DebugLogin()
{
    QString name = qEnvironmentVariable("USERNAME");

    if (name == "lukas")
    {
        ui->le_userName->setText("lwinter");
        ui->le_password->setText("30064461");
        return true;
    }

    return false;
}

void AMSMain::SetStatusLabel(const QString& text, const QColor& color)
{
    if (!_statusLabel)
    {
        return;
    }

    _statusLabel->setText(text);

    QPalette pal = _statusLabel->palette();
    pal.setColor(QPalette::WindowText, color);
    _statusLabel->setPalette(pal);
}

void AMSMain::ShowNextQueuedStatus()
{
    if (_statusQueue.empty())
    {
        if (_statusLabel)
        {
            _statusLabel->clear();
        }

        _statusTimer->stop();
        return;
    }

    const StatusItem item = _statusQueue.front();
    _statusQueue.pop();

    SetStatusLabel(item.text, item.color);
}

void AMSMain::ShowInstantStatus(const QString& text, const QColor& color)
{
    SetStatusLabel(text, color);

    QTimer::singleShot(_statusIntervalMs, this,
                       [this]()
                       {
                           ShowNextQueuedStatus();

                           if (!_statusQueue.empty())
                           {
                               _statusTimer->start();
                           }
                       });
}

void AMSMain::showEvent(QShowEvent* event)
{
    // later use
    QWidget::showEvent(event);
}

void AMSMain::closeEvent(QCloseEvent* event)
{
    ShutdownManager::Instance().RequestShutdown("MainWindowClose");
    QMainWindow::closeEvent(event);
}
