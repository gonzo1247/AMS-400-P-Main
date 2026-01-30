#pragma once

#include <QWidget>

#include "ArticleDatabaseSearchModel.h"
#include "ArticleManager.h"
#include "AssignEmployeeTableModel.h"
#include "EmployeeManager.h"
#include "ShowTicketDetailManager.h"
#include "SimilarTicketsTableModel.h"
#include "SparePartUsedManager.h"
#include "TicketAttachmentTableModel.h"
#include "TicketCommentTableModel.h"
#include "TicketSparePartsUsedModel.h"
#include "TicketTimelineTableModel.h"
#include "UploadAttachmentDialog.h"
#include "UploadLocalAttachmentDialog.h"
#include "ui_ShowTicketDetailWidget.h"

class TicketReportManager;

QT_BEGIN_NAMESPACE
namespace Ui { class ShowTicketDetailWidgetClass; };
QT_END_NAMESPACE

class ShowTicketDetailWidget : public QWidget
{
	Q_OBJECT

public:
	ShowTicketDetailWidget(QWidget *parent = nullptr);
	~ShowTicketDetailWidget() override;

	void LoadAndFillData(std::uint64_t ticketID);

private:
    void FillTicketDetailData();
    void ChangeEmployeeAssignment(bool assign);
    void ReloadTimeline();
    void ReloadCommentTable();

    void LoadSparePartTable(std::uint64_t ticketID);
    void LoadTicketReport(std::uint64_t ticketID);
    void LoadSimilarTickets(std::uint64_t ticketID);

    bool isTicketTitleModified() const { return ui->le_ticketTitle->isModified(); }
    bool isTicketDescriptionModified() const { return ui->pte_ticketDescription->document()->isModified(); }
    void setTicketTitleModified(bool modified) const { ui->le_ticketTitle->setModified(modified); }
    void setTicketDescriptionModified(bool modified) const { ui->pte_ticketDescription->document()->setModified(modified); }

    void ApplyRBACVisibility();    

	Ui::ShowTicketDetailWidgetClass *ui;
     QTimer *_loadingTimer{nullptr};
     int _loadingValue{0};

    std::uint64_t _ticketID;
     std::uint64_t _lastSimilarRequestTicketId = 0;

	TicketTimelineTableModel* _ticketTimelineModel;
    AssignEmployeeTableModel* _ticketAssignEmployeeModel;
    TicketAttachmentTableModel* _ticketAttachmentModel;
    TicketCommentTableModel* _ticketCommentModel;
    TicketSparePartsUsedModel* _ticketSparePartsModel;
    ArticleDatabaseSearchModel* _articleSearchModel;
    SimilarTicketsTableModel* _similarTicketModel;

    ShowTicketDetailWidget* _ticketDetailWidget;

    std::unique_ptr<ShowTicketDetailManager> _ticketDetailManager;
	std::unique_ptr<EmployeeManager> _employeeMgr;
    std::unique_ptr<SparePartUsedManager> _sparePartUsedMgr;
    std::unique_ptr<ArticleManager> _articleMgr;
    std::unique_ptr<TicketReportManager> _ticketReportMgr;
    std::unique_ptr<SimilarReportManager> _similarReportMgr;
	ShowTicketData _ticketData;
    TicketReportData _ticketReportData {};

    bool _reportExist;

private slots:
	void onPushAssignmentEmployee();
	void onPushUnassignmentEmployee();
    void onPushAddAttachment();
    void onAttachmentDoubleClicked(const QModelIndex& index);
    void onFileUploadFinish();
    void onPushAddComment();
    void onPushRemoveComment();
    void onPushSearchArticle();
    void OnSparePartRemoveRequested(std::uint64_t rowId);
    void OnSearchPartsDoubleClicked(const QModelIndex& index);
    void onPushSaveReport();
    void onPushSaveTicketDetails();
    void onPushCloseTicket();

signals:
    void TicketClosed();

};
