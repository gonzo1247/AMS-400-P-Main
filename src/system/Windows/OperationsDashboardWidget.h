#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "ContractorVisitManager.h"
#include "ContractorVisitModel.h"
#include "TableViewAutoScroller.h"
#include "TicketOperationDashboardModel.h"
#include "TicketColorAndWordWarpDelegate.h"

#include "ui_OperationsDashboardWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class OperationsDashboardWidgetClass;
};
QT_END_NAMESPACE

class OperationsDashboardWidget : public QWidget
{
    Q_OBJECT

   public:
    OperationsDashboardWidget(QWidget *parent = nullptr);
    ~OperationsDashboardWidget();

    // Setter
    void setTextPointSize(uint8_t size);
    void increaseTextPointSize();
    void decreaseTextPointSize();

    // Getter
    uint8_t GetTextPointSize();

    // Autoscroller
    TableViewAutoScroller *_ticketsScroller = nullptr;
    TableViewAutoScroller *_contractVisitsScroller = nullptr;

   private:
    void EnsureTicketUi();
    void LoadTicketsInitial();
    void LoadTicketsDelta();
    void StartAutoRefresh();
    void ApplyTicketViewLayout(bool tvMode);

    void LoadContractorInitial();
    void ApplyContractorVisitViewLayout(bool tvMode);

    Ui::OperationsDashboardWidgetClass *ui;
    TicketOperationDashboardModel *_ticketModel;
    std::unique_ptr<ShowTicketManager> _ticketMgr;

    std::unique_ptr<ContractorVisitManager> _contractVisitMgr;
    ContractorVisitModel *_contractVisitModel;

    std::atomic_bool _refreshActive{false};
    std::atomic_bool _refreshActiveContractor{false};
    std::string _lastSyncDb;

    QTimer *_refreshTimer = nullptr;
    QTimer *_openForTimer = nullptr;


    // Delegate
    TicketColorAndWordWarpDelegate *_ticketDelegate = nullptr;

   private slots:
    void onPushExitFullscreen();
};
