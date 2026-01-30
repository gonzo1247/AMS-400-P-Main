#pragma once

#include <QWidget>
#include <QPointer>

#include "ShowTicketDetailWidget.h"
#include "TicketTableModel.h"
#include "ui_ShowTicketWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ShowTicketWidgetClass; };
QT_END_NAMESPACE

class ShowTicketWidget : public QWidget
{
	Q_OBJECT

public:
	ShowTicketWidget(QWidget *parent = nullptr);
	~ShowTicketWidget();

private:
    void LoadTableData();
    void SetupTable();

	Ui::ShowTicketWidgetClass *ui;

	TicketTableModel *_ticketModel;
	std::unique_ptr<ShowTicketManager> _ticketMgr;
    QPointer<ShowTicketDetailWidget> _ticketDetailWidget;

    std::atomic_bool _refresh{false};

private slots:
    void onPushManualRefreshButton();

};

