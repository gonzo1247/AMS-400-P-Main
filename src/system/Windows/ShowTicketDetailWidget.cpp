#include "ShowTicketDetailWidget.h"

#include <QDesktopServices>
#include <QStandardPaths>
#include <QInputDialog>

#include "CostUnitDataHandler.h"
#include "Define.h"
#include "FileStorageManager.h"
#include "GlobalSignals.h"
#include "MessageBoxHelper.h"
#include "UploadAttachmentDialog.h"
#include "UploadLocalAttachmentDialog.h"
#include "UserCache.h"
#include "TicketSparePartsUsedActionDelegate.h"
#include "TicketReportManager.h"
#include "RBACVisibilityHelper.h"

ShowTicketDetailWidget::ShowTicketDetailWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ShowTicketDetailWidgetClass()), _ticketID(0),
_ticketTimelineModel(nullptr), _ticketAssignEmployeeModel(nullptr), _ticketAttachmentModel(nullptr), _ticketCommentModel(nullptr), _ticketSparePartsModel(nullptr), _articleSearchModel(nullptr),
      _ticketDetailManager(std::make_unique<ShowTicketDetailManager>()),
      _employeeMgr(nullptr),
      _sparePartUsedMgr(nullptr),
      _articleMgr(nullptr),
      _ticketReportMgr(nullptr),
      _reportExist(false),
      _similarTicketModel(nullptr),
      _similarReportMgr(nullptr),
      _ticketDetailWidget(nullptr)
{
	ui->setupUi(this);

    _loadingTimer = new QTimer(this);
    _loadingTimer->setInterval(30);  // animation speed

    connect(_loadingTimer, &QTimer::timeout, this, [this]()
    {
        // Simple looping animation 0 -> 100
        _loadingValue = (_loadingValue + 1) % 101;
        ui->pb_loadingIndicator->setValue(_loadingValue);
    });

    ui->pb_loadingIndicator->setVisible(false);

    {
        _ticketTimelineModel = new TicketTimelineTableModel(this);
        ui->tv_ticketData->setModel(_ticketTimelineModel);
        ui->tv_ticketData->setSortingEnabled(true);

        auto updateFilter = [this]()
        {
            QString filter;

            if (!ui->le_searchEmployee->text().trimmed().isEmpty())
                filter += ui->le_searchEmployee->text().trimmed() + " ";

            filter = filter.trimmed();

            _ticketAssignEmployeeModel->setFilterText(filter);
        };

        connect(ui->le_searchEmployee, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
    }

	{
	    // button connections
        connect(ui->pb_assignEmployee, &QPushButton::clicked, this,
                [this]()
                {
                    ChangeEmployeeAssignment(true);
                    ReloadTimeline();
                });
        connect(ui->pb_unassignEmployee, &QPushButton::clicked, this,
                [this]()
                {
                    ChangeEmployeeAssignment(false);
                    ReloadTimeline();
                });


        connect(ui->pb_uploadFile, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushAddAttachment);
        connect(ui->pb_commentAdd, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushAddComment);
        connect(ui->pb_commentRemove, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushRemoveComment);
	}

	{
	    connect(GlobalSignals::instance(), &GlobalSignals::FileUploadFinished, this, &ShowTicketDetailWidget::onFileUploadFinish);
	}

    connect(ui->tv_fileTable, &QTableView::doubleClicked, this, &ShowTicketDetailWidget::onAttachmentDoubleClicked);

    _ticketDetailManager->FillTicketStatusBox(ui->cb_newStatus);
    _ticketDetailManager->FillTicketPriorityBox(ui->cb_priority);

    connect(ui->cb_priority, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        if (index < 0)
            return;

        TicketPriority prio = static_cast<TicketPriority>(ui->cb_priority->itemData(index).toInt());
        ui->tb_priority->setText(GetTicketPriorityQString(prio));
        _ticketDetailManager->UpdatePriority(_ticketID, prio);

    });

    connect(ui->cb_newStatus, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        if (index < 0)
            return;

        TicketStatus status = static_cast<TicketStatus>(ui->cb_newStatus->itemData(index).toInt());
        ui->tb_currentStatus->setText(GetTicketStatusQString(status));
        _ticketDetailManager->UpdateStatus(_ticketID, status);
    });

    connect(ui->pb_searchParts, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushSearchArticle);
    connect(ui->le_searchParts, &QLineEdit::returnPressed, this, &ShowTicketDetailWidget::onPushSearchArticle);

    auto* delegate = new TicketSparePartsUsedActionDelegate(/*permId*/ 0, ui->tv_addedParts);
    ui->tv_addedParts->setItemDelegateForColumn(static_cast<int>(TicketSparePartsUsedModel::Column::Action), delegate);
    connect(delegate, &TicketSparePartsUsedActionDelegate::removeRequested, this, &ShowTicketDetailWidget::OnSparePartRemoveRequested);
    connect(ui->tv_searchParts, &QTableView::doubleClicked, this, &ShowTicketDetailWidget::OnSearchPartsDoubleClicked);
    connect(ui->pb_saveReport, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushSaveReport);

    connect(ui->tv_simular_issues, &QTableView::doubleClicked, this,
            [this](const QModelIndex& idx)
            {
                const auto* row = _similarTicketModel->GetRow(idx.row());
                if (!row)
                    return;

                if (!_ticketDetailWidget)
                {
                    _ticketDetailWidget = new ShowTicketDetailWidget(this);
                    _ticketDetailWidget->setAttribute(Qt::WA_DeleteOnClose);
                    _ticketDetailWidget->setWindowFlag(Qt::Window, true);
                }

                _ticketDetailWidget->LoadAndFillData(row->ticketId);
                _ticketDetailWidget->show();
            });


    connect(ui->pb_save, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushSaveTicketDetails);
    connect(ui->pb_close, &QPushButton::clicked, this, &ShowTicketDetailWidget::onPushCloseTicket);

    ApplyRBACVisibility();
}

ShowTicketDetailWidget::~ShowTicketDetailWidget()
{
	delete ui;
}

void ShowTicketDetailWidget::LoadAndFillData(std::uint64_t ticketID)
{    
    if (!_ticketDetailManager)
        _ticketDetailManager = std::make_unique<ShowTicketDetailManager>();

    if (!_employeeMgr)
        _employeeMgr =  std::make_unique<EmployeeManager>();

    if (!_ticketTimelineModel)
    {
        _ticketTimelineModel = new TicketTimelineTableModel(this);
        ui->tv_ticketData->setModel(_ticketTimelineModel);
        ui->tv_ticketData->setSortingEnabled(true);
    }

    if (!_ticketAssignEmployeeModel)
    {
        _ticketAssignEmployeeModel = new AssignEmployeeTableModel(this);
        ui->tv_employee->setModel(_ticketAssignEmployeeModel);
        ui->tv_employee->setSortingEnabled(true);
    }

    if (!_ticketAttachmentModel)
    {
        _ticketAttachmentModel = new TicketAttachmentTableModel(this);
        ui->tv_fileTable->setModel(_ticketAttachmentModel);
        ui->tv_fileTable->setSortingEnabled(true);
    }

    if (!_ticketCommentModel)
    {
        _ticketCommentModel = new TicketCommentTableModel(this);
        ui->tv_commentTable->setModel(_ticketCommentModel);
        ui->tv_commentTable->setSortingEnabled(true);
    }

    _ticketID = ticketID;

    auto task = [this, ticketID]()
    {
        QMetaObject::invokeMethod(
        this, [this]()
        {
                ui->pb_loadingIndicator->setVisible(true);
                ui->pb_loadingIndicator->setRange(0, 100);

                _loadingValue = 0;
                ui->pb_loadingIndicator->setValue(_loadingValue);

                if (_loadingTimer && !_loadingTimer->isActive())
                    _loadingTimer->start();
        }, Qt::QueuedConnection);


        _ticketDetailManager->LoadTicketDetails(ticketID); // Load ticket details using the provided ticketID
        _ticketData = _ticketDetailManager->GetTicketData();    // Assuming a method to get the ticket data
        auto localMap = _ticketData;

        _employeeMgr->LoadEmployeeData();
        auto empTable = _employeeMgr->LoadEmployeeDataForDetails(ticketID);


        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap), empTable = std::move(empTable)]()
            {
                // Ensure model exists once
                if (!_ticketTimelineModel)
                {
                    _ticketTimelineModel = new TicketTimelineTableModel(this);
                    ui->tv_ticketData->setModel(_ticketTimelineModel);
                    ui->tv_ticketData->setSortingEnabled(true);
                }

                _ticketTimelineModel->setEntries(localMap.timeline);
                FillTicketDetailData();

                if (!_ticketAssignEmployeeModel)
                {
                    _ticketAssignEmployeeModel = new AssignEmployeeTableModel(this);
                    ui->tv_employee->setModel(_ticketAssignEmployeeModel);
                    ui->tv_employee->setSortingEnabled(true);
                }

                _ticketAssignEmployeeModel->setRows(empTable);

                ui->tv_employee->hideColumn(0);  // hide ID column

                
                if (!_ticketAttachmentModel)
                {
                    _ticketAttachmentModel = new TicketAttachmentTableModel(this);
                    ui->tv_fileTable->setModel(_ticketAttachmentModel);
                    ui->tv_fileTable->setSortingEnabled(true);
                }

                _ticketAttachmentModel->setAttachments(localMap.ticketAttachment);

                {
                    auto* h = ui->tv_fileTable->horizontalHeader();
                    h->setSectionResizeMode(0, QHeaderView::Stretch);
                    h->setSectionResizeMode(1, QHeaderView::Stretch);
                    h->setSectionResizeMode(2, QHeaderView::ResizeToContents);
                    h->setSectionResizeMode(3, QHeaderView::ResizeToContents);
                    h->setSectionResizeMode(4, QHeaderView::ResizeToContents);
                    h->setSectionResizeMode(5, QHeaderView::ResizeToContents);
                }

                if (!_ticketCommentModel)
                {
                    _ticketCommentModel = new TicketCommentTableModel(this);
                    ui->tv_commentTable->setModel(_ticketCommentModel);
                    ui->tv_commentTable->setSortingEnabled(true);
                }

                _ticketCommentModel->setData(localMap.ticketComment);

                {
                    ui->tv_commentTable->setColumnHidden(0, true);  // ID
                    ui->tv_commentTable->setColumnHidden(2, true);  // updated_at
                    ui->tv_commentTable->setColumnHidden(4, true);  // internal
                    ui->tv_commentTable->setColumnHidden(5, true);  // deleted
                }

                ui->pb_loadingIndicator->setVisible(false);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);

    LoadSparePartTable(ticketID);
    LoadTicketReport(ticketID);
    LoadSimilarTickets(ticketID);
}

void ShowTicketDetailWidget::FillTicketDetailData()
{ 
    ui->tb_ticketID->setText(QString::number(_ticketData.ticketInfo.id));

    if (auto userData = GetUserDataByID(_ticketData.ticketInfo.creatorUserID))
        ui->tb_ticketCreator->setText(QString::fromStdString(userData->GetUserName()));
    else
        ui->tb_ticketCreator->setText("Unknown User");

    ui->tb_createdAt->setText(Util::FormatDateTimeQString(_ticketData.ticketInfo.createdAt));
    ui->tb_currentStatus->setText(GetTicketStatusQString(static_cast<TicketStatus>(_ticketData.ticketInfo.currentStatus)));
    ui->tb_costUnit->setText(QString::fromStdString(CostUnitDataHandler::instance().GetCostUnitNameByInternalId(_ticketData.ticketInfo.costUnitID)));
    ui->tb_room->setText(QString::fromStdString(_ticketData.ticketInfo.area));
    ui->tb_priority->setText(GetTicketPriorityQString(static_cast<TicketPriority>(_ticketData.ticketInfo.priority)));

    const QString callerName = tr("%1 (%2)").arg(QString::fromStdString(_ticketData.callerInfo.name), QString::fromStdString(_ticketData.callerInfo.phone));
    ui->tb_reporter->setText(callerName);

    ui->le_ticketTitle->setText(QString::fromStdString(_ticketData.ticketInfo.title));
    ui->le_ticketTitle->setModified(false);
    ui->pte_ticketDescription->setPlainText(QString::fromStdString(_ticketData.ticketInfo.description));
    ui->pte_ticketDescription->document()->setModified(false);

    ui->tb_entityName->setText(QString::fromStdString(_ticketData.machineInfo.MachineName));
    ui->tb_entityLineID->setText(_ticketDetailManager->LoadMachineLineName(_ticketData.machineInfo.LineID));
    ui->tb_entityType->setText(_ticketDetailManager->LoadMachineTypeName(_ticketData.machineInfo.MachineTypeID));
    ui->tb_entityNumber->setText(QString::fromStdString(_ticketData.machineInfo.MachineNumber));
    ui->tb_entityManufacturer->setText(QString::fromStdString(_ticketData.machineInfo.ManufacturerMachineNumber));
    ui->lb_ph_manufacturerName->setText(_ticketDetailManager->LoadManufacturerName(_ticketData.machineInfo.ManufacturerID));


    ui->cb_priority->setCurrentIndex(static_cast<int>(_ticketData.ticketInfo.priority));
    ui->cb_newStatus->setCurrentIndex(static_cast<int>(_ticketData.ticketInfo.currentStatus));
}

void ShowTicketDetailWidget::ChangeEmployeeAssignment(bool assign)
{
    if (!_employeeMgr)
        _employeeMgr = std::make_unique<EmployeeManager>();

    QModelIndex selectedIndex = ui->tv_employee->selectionModel()->currentIndex();

    if (!selectedIndex.isValid())
    {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select an employee."));
        return;
    }

    const auto employeeIdOpt = _ticketAssignEmployeeModel->employeeIdForIndex(selectedIndex);
    if (!employeeIdOpt.has_value())
    {
        QMessageBox::warning(this, tr("Error"), tr("Could not determine the selected employee."));
        return;
    }

    const std::uint32_t employeeId = *employeeIdOpt;
    const bool isCurrentlyActive = _ticketAssignEmployeeModel->isEmployeeActiveByIndex(selectedIndex);

    if (assign && isCurrentlyActive)
    {
        QMessageBox::information(this, tr("Already Assigned"), tr("This employee is already assigned to the ticket."));
        return;
    }

    if (!assign && !isCurrentlyActive)
    {
        QMessageBox::information(this, tr("Not Assigned"), tr("This employee is not assigned to the ticket."));
        return;
    }

    if (assign)
    {
        _employeeMgr->AddEmployeeAssignment(_ticketID, employeeId, ui->le_comment->text().toStdString());
    }
    else
    {
        _employeeMgr->RemoveEmployeeAssignment(_ticketID, employeeId, ui->le_comment->text().toStdString());
    }

    _ticketAssignEmployeeModel->setEmployeeActiveByIndex(selectedIndex, assign);
}

void ShowTicketDetailWidget::ReloadTimeline()
{
    const auto id = _ticketID;

    auto task = [this, id]()
    {
        _ticketDetailManager->LoadTicketDetails(id);
        auto updated = _ticketDetailManager->GetTicketData();

        QMetaObject::invokeMethod(
            this,
            [this, updated = std::move(updated)]()
            {
                if (_ticketTimelineModel)
                    _ticketTimelineModel->setEntries(updated.timeline);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::ReloadCommentTable()
{
    const auto id = _ticketID;

    auto task = [this, id]()
    {
        _ticketDetailManager->LoadTicketDetails(id);
        auto updated = _ticketDetailManager->GetTicketData();

        QMetaObject::invokeMethod(
            this,
            [this, updated = std::move(updated)]()
            {
                if (_ticketCommentModel)
                    _ticketCommentModel->setData(updated.ticketComment);

                if (_ticketTimelineModel)
                    _ticketTimelineModel->setEntries(updated.timeline);

                ui->tv_commentTable->setColumnHidden(0, true); // ID
                ui->tv_commentTable->setColumnHidden(2, true); // updated_at
                ui->tv_commentTable->setColumnHidden(4, true); // internal
                ui->tv_commentTable->setColumnHidden(5, true);  // deleted

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::LoadSparePartTable(std::uint64_t ticketID)
{
    if (!_ticketSparePartsModel)
    {
        _ticketSparePartsModel = new TicketSparePartsUsedModel(this);
        ui->tv_addedParts->setModel(_ticketSparePartsModel);
        ui->tv_addedParts->setSortingEnabled(true);
    }

    if (!_ticketSparePartsModel)
        _sparePartUsedMgr = std::make_unique<SparePartUsedManager>();

    auto task = [this, ticketID]()
    {
        auto spareParts = _sparePartUsedMgr->LoadSparePartsDataForTicket(ticketID);

        QMetaObject::invokeMethod(
            this,
            [this, spareParts = std::move(spareParts)]()
            {
                _ticketSparePartsModel->SetData(spareParts);
                ui->tv_addedParts->hideColumn(to_underlying(TicketSparePartsUsedModel::Column::Note));
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::LoadTicketReport(std::uint64_t ticketID)
{
    auto task = [this, ticketID]()
    {
        if (!_ticketReportMgr)
            _ticketReportMgr = std::make_unique<TicketReportManager>();

        auto report = _ticketReportMgr->LoadTicketReport(ticketID);
        _ticketReportData = report;

        QMetaObject::invokeMethod(
            this,
            [this, report = std::move(report)]()
            {
                ui->wi_reportFormular->setHtml(QString::fromStdString(report.reportHTML));
                ui->pb_saveReport->setText(tr("Update Report"));

                if (report.updatedAt.has_value())
                    ui->dte_lastUpdate->setDateTime(Util::ConvertToQDateTime(report.updatedAt.value()));
                else
                    ui->dte_lastUpdate->setDateTime(QDateTime::currentDateTime());

                ui->le_createdBy->setText(QString::fromStdString(report.createdByUserName));

                _reportExist = true;

            },
            Qt::QueuedConnection);
        
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::LoadSimilarTickets(std::uint64_t ticketID)
{
    LOG_DEBUG("Similar: FindSimilarTickets ticketId={}", ticketID);


    if (!_similarTicketModel)
    {
        _similarTicketModel = new SimilarTicketsTableModel(this);

        ui->tv_simular_issues->setModel(_similarTicketModel);

        ui->tv_simular_issues->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tv_simular_issues->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tv_simular_issues->setSortingEnabled(true);
        ui->tv_simular_issues->horizontalHeader()->setStretchLastSection(true);
        ui->tv_simular_issues->setAlternatingRowColors(true);
    }

    const std::uint64_t requestTicketId = ticketID;
    _lastSimilarRequestTicketId = requestTicketId;

    QPointer<ShowTicketDetailWidget> self(this);

    auto task = [self, requestTicketId]()
    {
        if (!self)
            return;

        std::vector<SimilarReportResult> results;
        try
        {
            SimilarReportManager mgr;
            results = mgr.FindSimilarTickets(requestTicketId, 15);
        }
        catch (...)
        {
            results.clear();
        }

        QMetaObject::invokeMethod(self, [self, requestTicketId, results = std::move(results)]() mutable
            {
                if (!self)
                    return;

                if (self->_lastSimilarRequestTicketId != requestTicketId)
                    return;

                self->_similarTicketModel->SetResults(std::move(results));
                self->ui->tv_simular_issues->sortByColumn(SimilarTicketsTableModel::ColumnScore, Qt::DescendingOrder);
                self->ui->tv_simular_issues->resizeColumnsToContents();
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(task, this);

}

void ShowTicketDetailWidget::ApplyRBACVisibility()
{
    ChangeVisibilityAndTooltip(Permission::RBAC_CHANGE_TICKET_TITLE_DESCRIPTION, ui->le_ticketTitle);
    ChangeVisibilityAndTooltip(Permission::RBAC_CHANGE_TICKET_TITLE_DESCRIPTION, ui->pte_ticketDescription);
    ChangeVisibilityAndTooltip(Permission::RBAC_CHANGE_TICKET_TITLE_DESCRIPTION, ui->pb_save);
    ChangeVisibilityAndTooltip(Permission::RBAC_CHANGE_TICKET_PRIORITY, ui->cb_priority);
    ChangeVisibilityAndTooltip(Permission::RBAC_CHANGE_TICKET_STATUS, ui->cb_newStatus);
    ChangeVisibilityAndTooltip(Permission::RBAC_ASSIGN_UNASSIGN_EMPLOYEE, ui->pb_assignEmployee);
    ChangeVisibilityAndTooltip(Permission::RBAC_ASSIGN_UNASSIGN_EMPLOYEE, ui->pb_unassignEmployee);
    ChangeVisibilityAndTooltip(Permission::RBAC_ASSIGN_UNASSIGN_EMPLOYEE, ui->le_searchEmployee);
    ChangeVisibilityAndTooltip(Permission::RBAC_UPLOAD_FILES, ui->pb_uploadFile);
    ChangeVisibilityAndTooltip(Permission::RBAC_ADD_TICKET_COMMENT, ui->pb_commentAdd);
    ChangeVisibilityAndTooltip(Permission::RBAC_ADD_TICKET_COMMENT, ui->pb_commentRemove);
    ChangeVisibilityAndTooltip(Permission::RBAC_ADD_TICKET_COMMENT, ui->le_commentText);
    ChangeVisibilityAndTooltip(Permission::RBAC_CREATE_REPORT, ui->pb_saveReport);
    ChangeVisibilityAndTooltip(Permission::RBAC_CLOSE_TICKET, ui->pb_close);
}

void ShowTicketDetailWidget::onPushAssignmentEmployee()
{    
    ChangeEmployeeAssignment(true);
}

void ShowTicketDetailWidget::onPushUnassignmentEmployee()
{
    ChangeEmployeeAssignment(false);
}

void ShowTicketDetailWidget::onPushAddAttachment()
{
    if (GetSettings().isNetworkFileEnabled())
    {
        UploadAttachmentDialog dlg(_ticketID, this);
        dlg.exec();
    }

    if (GetSettings().isLocalFileEnabled())
    {
        UploadLocalAttachmentDialog dlg(_ticketID, this);
        dlg.exec();
    }
}

void ShowTicketDetailWidget::onAttachmentDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    auto* model = qobject_cast<TicketAttachmentTableModel*>(ui->tv_fileTable->model());
    if (!model)
        return;

    auto infoOpt = model->getAttachment(index.row());
    if (!infoOpt.has_value())
        return;

    const auto& att = *infoOpt;

    if (GetSettings().isNetworkFileEnabled())
    {
        const QString basePath = GetSettings().getNetworkShareData().networkSharePath;

        QDir baseDir(basePath);
        const QString fullPath = baseDir.filePath(QString::fromStdString(att.filePath));

        if (!QFileInfo::exists(fullPath))
        {
            MessageBoxHelper::ShowErrorMessage(tr("File not found:\n%1").arg(fullPath));
            return;
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
        return;
    }

    // Local/encrypted variant
    if (GetSettings().isLocalFileEnabled())
    {
        auto localFileData = GetSettings().getLocalFileData();
        const QString rootPath = localFileData.rootPath;
        FileStorageManager storage(rootPath);

        // Build temp target
        const QString tempRoot =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/AMS_Attachments");

        QDir().mkpath(tempRoot);

        // Preferably use original names, otherwise storedFileName
        QString targetName;

        if (!att.originalFilename.empty())
            targetName = QString::fromStdString(att.originalFilename);
        else if (!att.storedFileName.empty())
            targetName = QString::fromStdString(att.storedFileName);
        else
            targetName = QStringLiteral("attachment_%1").arg(att.id);

        const QString tempFullPath = QDir(tempRoot).filePath(targetName);

        // Restore file from encrypted store
        if (!storage.RestoreAttachment(att.id, tempFullPath))
        {
            MessageBoxHelper::ShowErrorMessage(
                tr("Could not open attachment.\nDecryption or file restoration failed."));
            return;
        }

        if (!QFileInfo::exists(tempFullPath))
        {
            MessageBoxHelper::ShowErrorMessage(tr("Temporary file not found:\n%1").arg(tempFullPath));
            return;
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(tempFullPath));
    }
}

void ShowTicketDetailWidget::onFileUploadFinish()
{
    const auto id = _ticketID;

    auto task = [this, id]()
    {
        _ticketDetailManager->LoadTicketDetails(id);
        auto updated = _ticketDetailManager->GetTicketData();

        QMetaObject::invokeMethod(
            this,
            [this, updated = std::move(updated)]()
            {
                if (_ticketAttachmentModel)
                    _ticketAttachmentModel->setAttachments(updated.ticketAttachment);

                if (_ticketTimelineModel)
                    _ticketTimelineModel->setEntries(updated.timeline);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::onPushAddComment()
{
    _ticketDetailManager->AddNewComment(_ticketID, ui->le_commentText->text());
    ReloadCommentTable();
    ui->le_commentText->clear();
}

void ShowTicketDetailWidget::onPushRemoveComment()
{
    const QModelIndex index = ui->tv_commentTable->currentIndex();
    if (!index.isValid())
        return;

    auto commentOpt = _ticketCommentModel->getComment(index.row());
    if (!commentOpt.has_value())
        return;

    const auto& c = *commentOpt;

    const std::uint64_t commentID = c.id;

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Delete comment"), tr("Do you really want to delete this comment?"), QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    _ticketDetailManager->RemoveComment(commentID);
    ReloadCommentTable();
}

void ShowTicketDetailWidget::onPushSearchArticle()
{
    if (!_articleSearchModel)
    {
        _articleSearchModel = new ArticleDatabaseSearchModel(this);
        ui->tv_searchParts->setModel(_articleSearchModel);
        ui->tv_searchParts->setSortingEnabled(true);
    }

    if (!_articleMgr)
        _articleMgr = std::make_unique<ArticleManager>();

    auto tokenString = ui->le_searchParts->text();

    auto task = [this, tokenString]()
    {
        auto articleData = _articleMgr->LoadArticleSearch(tokenString);

        QMetaObject::invokeMethod(this, [this, articleData = std::move(articleData)]()
            {
                _articleSearchModel->SetData(articleData);
                ui->tv_searchParts->hideColumn(static_cast<int>(ArticleDatabaseSearchModel::Column::ArticleId));  // hide ID column
            }, Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void ShowTicketDetailWidget::OnSparePartRemoveRequested(std::uint64_t rowId)
{
    const auto btn = QMessageBox::question(this, tr("Remove spare part"),
                                           tr("Do you really want to remove this spare part entry from the ticket?\n\n"
                                              "This will NOT book the part back into IMS."),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (btn != QMessageBox::Yes)
    {
        return;
    }

    const std::uint64_t currentUserId = GetUser().GetUserID();

    if (!_sparePartUsedMgr->SoftDeleteSparePartUsedById(rowId, currentUserId))
    {
        QMessageBox::warning(this, tr("Remove failed"),
                             tr("The entry could not be removed. Please try again or contact an administrator."));
        return;
    }

    if (_ticketSparePartsModel)
    {
        _ticketSparePartsModel->RemoveById(rowId);
    }
}

void ShowTicketDetailWidget::OnSearchPartsDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    const QModelIndex idIndex = index.siblingAtColumn(0);
    const QVariant v = idIndex.data(Qt::DisplayRole);
    if (!v.isValid())
    {
        return;
    }

    bool ok = false;
    const std::uint64_t articleId = v.toULongLong(&ok);
    if (!ok || articleId == 0)
    {
        return;
    }

    // Article Name
    const QModelIndex idIndexAN = index.siblingAtColumn(1);
    const QVariant vAN = idIndexAN.data(Qt::DisplayRole);
    if (!vAN.isValid())
    {
        return;
    }

    const QString articleName = vAN.toString();

    int qty = QInputDialog::getInt(this, tr("Quantity"), tr("Enter quantity to add:"), 1, 1, 65535, 1, &ok);

    if (!ok)
    {
        return;
    }

    const std::uint64_t ticketId = _ticketID;
    const std::uint32_t machineId = _ticketData.machineInfo.ID;

    const std::uint64_t currentUserId = GetUser().GetUserID();
    const std::string currentUserName = GetUser().GetUserName();

    auto insertedId = _sparePartUsedMgr->InsertSparePartUsed(ticketId, machineId, articleId, static_cast<std::uint16_t>(qty),
                                                    "pcs", "", currentUserId, currentUserName);

    if (!insertedId.has_value())
    {
        QMessageBox::warning(this, tr("Insert failed"), tr("Could not add the spare part to the ticket."));
        return;
    }

    TicketSparePartUsedInformation row;
    row.id = *insertedId;
    row.ticketID = ticketId;
    row.machineID = machineId;
    row.articleID = articleId;
    row.quantity = static_cast<std::uint16_t>(qty);
    row.unit = "pcs";
    row.note = "";
    row.createdByUserID = currentUserId;
    row.createdByUserName = currentUserName;
    row.createdAt = Util::GetCurrentSystemPointTime();

    SparePartsTable newPart {};
    newPart.spareData = row;
    newPart.articleName = articleName.toStdString();

    _ticketSparePartsModel->AppendRow(std::move(newPart));
}

void ShowTicketDetailWidget::onPushSaveReport()
{
    if (!ui->wi_reportFormular->hasUnsavedChanges())
    {
        QMessageBox::information(this, tr("No changes"), tr("There are no unsaved changes to save."));
        return;
    }

    bool ok = false;

    if (!_reportExist)
    {
        _ticketReportData.ticketID = _ticketID;
        _ticketReportData.reportHTML = ui->wi_reportFormular->toHtml().toStdString();
        _ticketReportData.reportPlain = ui->wi_reportFormular->toPlain().toStdString();
        _ticketReportData.createdByUserID = GetUser().GetUserID();
        _ticketReportData.createdByUserName = GetUser().GetUserName();

        ok = _ticketReportMgr->SaveNewReport(_ticketReportData);
    }
    else
    {
        _ticketReportData.reportHTML = ui->wi_reportFormular->toHtml().toStdString();
        _ticketReportData.reportPlain = ui->wi_reportFormular->toPlain().toStdString();
        _ticketReportData.updatedAt = Util::GetCurrentSystemPointTime();
        ok = _ticketReportMgr->SaveReportUpdate(_ticketReportData);
    }

    if (!ok)
    {
        QMessageBox::warning(this, tr("Save failed"), tr("Could not save the report."));
        return;
    }

    QMessageBox::information(this, tr("Report saved"), tr("The report has been saved successfully."));

    ui->wi_reportFormular->setMarkAsSaved();
}

void ShowTicketDetailWidget::onPushSaveTicketDetails()
{
    if (!isTicketTitleModified() && !isTicketDescriptionModified())
    {
        QMessageBox::information(this, tr("No changes"), tr("There are no changes to save."));
        return;
    }

    bool ok = false;

    if (isTicketTitleModified())
    {
        const QString newTitle = ui->le_ticketTitle->text();
        ok = _ticketDetailManager->UpdateTicketTitle(_ticketID, newTitle);
        setTicketTitleModified(false);
    }

    if (isTicketDescriptionModified())
    {
        const QString newDescription = ui->pte_ticketDescription->toPlainText();
        ok = _ticketDetailManager->UpdateTicketDescription(_ticketID, newDescription);
        setTicketDescriptionModified(false);
    }

    if (!ok)
    {
        QMessageBox::warning(this, tr("Save failed"), tr("Could not save the changes to the ticket."));
        return;
    }

    QMessageBox::information(this, tr("Save successful"), tr("The changes to the ticket have been saved successfully."));

}

void ShowTicketDetailWidget::onPushCloseTicket()
{
    if (QMessageBox::warning(this, tr("Close Ticket"), tr("Are you sure you want to close this ticket?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        _ticketDetailManager->CloseTicket(_ticketID);
        emit TicketClosed();
        close();
    }
}
