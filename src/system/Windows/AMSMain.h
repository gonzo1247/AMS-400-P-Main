#pragma once

#include <QMainWindow>
#include <queue>

#include "AssetManager.h"
#include "CallerManagerWidget.h"
#include "CreateTicketWidget.h"
#include "EmployeeManagerWidget.h"
#include "OperationsDashboardControlWidget.h"
#include "OperationsDashboardWidget.h"
#include "ShowTicketWidget.h"
#include "UserManagement.h"
#include "WindowTest.h"
#include "ui_AMSMain.h"


QT_BEGIN_NAMESPACE
namespace Ui
{
    class AMSMainClass;
};
QT_END_NAMESPACE

class QCloseEvent;
class ContractorWorkerData;
class ContractVisit;

enum StackWidgetPagesAMSMain
{
    PAGE_LOGIN = 0,
    PAGE_MAIN = 1,
    PAGE_EXTERNAL_WIDGET = 2
};

struct StatusItem
{
    QString text;
    QColor color;
};

class AMSMain : public QMainWindow
{
    Q_OBJECT

   public:
    AMSMain(QWidget* parent = nullptr);
    ~AMSMain();

    void CreateConnections();

   private:
    void LogoutAndReset();
    void ApplyRBACVisibility();

    Ui::AMSMainClass* ui;
    QLabel* _statusLabel{nullptr};

    CallerManagerWidget* _callerMgrWidget;
    CallerManagerWidget* _callerMgrWidgetExternal;
    QPointer<CreateTicketWidget> _createTicketWidgetExternal;
    CreateTicketWidget* _createTicketWidget = nullptr;
    EmployeeManagerWidget* _employeeMgrWidget;
    EmployeeManagerWidget* _employeeMgrWidgetExternal;
    ShowTicketWidget* _showTicketWidget;
    ShowTicketWidget* _showTicketWidgetExternal;
    OperationsDashboardWidget* _operationsDashboardWidget;
    OperationsDashboardWidget* _operationsDashboardWidgetExternal;
    OperationsDashboardControlWidget* _operationsDashboardControlWidget;
    AssetManager* _assetManagerWidget;
    ContractorWorkerData* _contractorWorkerDataWidget{nullptr};
    ContractVisit* _contractorVisitWidget{nullptr};

    std::unique_ptr<User> _userMgr;

    void DisplayExternalWidget(QWidget* widget, bool deletePreviousWidget = false);
    void DisplayMainPage();

    // Status Message
    void ShowNextQueuedStatus();
    void ShowInstantStatus(const QString& text, const QColor& color);
    void SetStatusLabel(const QString& text, const QColor& color);
    void EnsureStatusTimer();

    bool DebugLogin();

    QTimer* _statusTimer = nullptr;
    std::queue<StatusItem> _statusQueue;

    int _statusIntervalMs = 3000;

   private slots:
    void onPushLogin();
    void onPushLogout();
    void onPushCallerManager();
    void onPushCreateTicket();
    void onPushShowTicket();
    void onPushEmployeeManager();
    void onCallerReturnToMainPage();
    void onCreateTicketBack();
    void onPushShowTV();
    void onPushShowAssetManager();
    void onPushContractorWorkerOpen();
    void onPushContractorVisit();

    // Status message
    void ReceiveStatusMessage(const std::string& message, StatusMessageQueue queue, const QColor& color);

   protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
};
