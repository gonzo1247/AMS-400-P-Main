#include "CreateTicketWidget.h"

#include <QShortcut>

#include "AMSMain.h"
#include "CostUnitDataHandler.h"
#include "GlobalSignals.h"
#include "MessageBoxHelper.h"
#include "Util.h"

/*
 ui->w_caller
 ui->w_department
 ui->w_line
 ui->w_employee
 are promoted to StepWidget(inherited from StatusBarWidget - qlementine)!!!!!
 */

using oclero::qlementine::StatusBadge;

CreateTicketWidget::CreateTicketWidget(QWidget *parent) : QWidget(parent) , ui(new Ui::CreateTicketWidgetClass()), _callerModel(nullptr), _employeeModel(nullptr), _callerMgr(nullptr), _employeeMgr(nullptr),
_costUnitModel(nullptr), _machineListModel(nullptr), _machineListMgr(nullptr), _createTicketMgr(nullptr), _saveIsPossible(false)
{
	ui->setupUi(this);
    Initialize();
    InitConnections();

    {
        connect(ui->pb_reset, &QPushButton::clicked, this, &CreateTicketWidget::onPushReset);
        connect(ui->pb_save, &QPushButton::clicked, this, &CreateTicketWidget::onPushSave);
        connect(ui->pb_stepBack, &QPushButton::clicked, this, &CreateTicketWidget::onEscPressed);
    }

    {
        connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this), &QShortcut::activated, this, [this]() { onPushSave(); });
        connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this, &CreateTicketWidget::onEscPressed);
        connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this), &QShortcut::activated, this, &CreateTicketWidget::onPushReset);
        auto* scFind = new QShortcut(QKeySequence(QKeySequence::Find), this);
        scFind->setContext(Qt::WidgetWithChildrenShortcut);

        connect(scFind, &QShortcut::activated, this,
                [this]()
                {
                    ui->le_search->setFocus();
                    ui->le_search->selectAll();
                });

        // Ctrl + T -> focus ticket title
        auto* scTitle = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
        scTitle->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scTitle, &QShortcut::activated, this,
                [this]()
                {
                    if (ui->le_ticketTitle)
                    {
                        ui->le_ticketTitle->setFocus();
                        ui->le_ticketTitle->selectAll();
                    }
                });

        // Ctrl + D -> focus ticket description
        auto* scDesc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), this);
        scDesc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scDesc, &QShortcut::activated, this,
                [this]()
                {
                    if (ui->pte_ticketDescription)
                    {
                        ui->pte_ticketDescription->setFocus();
                        ui->pte_ticketDescription->selectAll();
                    }
                });
    }
}

CreateTicketWidget::~CreateTicketWidget()
{
	delete ui;
}

void CreateTicketWidget::setModels(CallerTableModel* callerModel, CostUnitTableModel* costUnitModel, MachineListTableModel* machineListModel, EmployeeTableModel* employeeModel)
{
    _callerModel = callerModel;
    _costUnitModel = costUnitModel;
    _machineListModel = machineListModel;
    _employeeModel = employeeModel;

    SetStep(TicketStep::STEP_1_CALLER);
}

void CreateTicketWidget::InitSteps()
{

}

void CreateTicketWidget::SetStep(TicketStep step)
{
    _currentStep = step;

    // Decide which model and which filter function to use.
    switch (_currentStep)
    {
        case TicketStep::STEP_1_CALLER:
        {
            LoadCallerTable();
            _currentModel = _callerModel;
            _currentFilterSetter = [this](const QString& text)
            {
                if (_callerModel)
                {
                    _callerModel->setFilterText(text);
                }
            };
            ui->gB_searchResult->setTitle(tr("Please select the Caller: "));
            if (_currentModel->rowCount() > 0)
            {
                ui->tv_lookupTable->clearSelection();
                ui->tv_lookupTable->selectRow(1);
            }
            break;
        }
        case TicketStep::STEP_2_DEPARTMENT:
        {
            LoadCostUnitTable();
            _currentModel = _costUnitModel;
            _currentFilterSetter = [this](const QString& text)
            {
                if (_costUnitModel)
                {
                    _costUnitModel->setFilterText(text);
                }
            };
            ui->gB_searchResult->setTitle(tr("Please select the Costunit: "));
            break;
        }
        case TicketStep::STEP_3_LINE_OR_MACHINE:
        {
            LoadMachineListTable();
            _currentModel = _machineListModel;
            _currentFilterSetter = [this](const QString& text)
            {
                if (_machineListModel)
                {
                    _machineListModel->setFilterText(text);
                }
            };
            ui->gB_searchResult->setTitle(tr("Please select the Line, Machine or a Room: "));
            break;
        }
        case TicketStep::STEP_5_EMPLOYEE:
        {
            LoadEmployeeTable();
            _currentModel = _employeeModel;
            _currentFilterSetter = [this](const QString& text)
            {
                if (_employeeModel)
                {
                    _employeeModel->setFilterText(text);
                }
            };
            ui->gB_searchResult->setTitle(tr("Please assign a Technican: "));
            break;
        }
    }


    // Apply model to table view.
    if (_currentModel)
        ui->tv_lookupTable->setModel(_currentModel);

    ResetColumnHidden();

    // Reset search input for new step.
    if (!ui->le_search->text().isEmpty())
        ui->le_search->clear();

    if (_currentFilterSetter)
    {
        _currentFilterSetter(QString());
    }

    // Update badges (Caller/Department/Line/Employee) etc.

    QTimer::singleShot(500, [this]()
        {
        UpdateStepUI();
        UpdateTableStyle();
        });
    
}

void CreateTicketWidget::UpdateStepUI()
{
    using Badge = oclero::qlementine::StatusBadge;

    const int current = static_cast<int>(_currentStep);

    auto apply = [current](int stepIndex, StepWidget* widget)
    {
        if (!widget)
            return;

        if (stepIndex < current)
        {
            widget->setBadge(Badge::Success);  // done
        }
        else if (stepIndex == current)
            widget->setBadge(Badge::Info);  // current
        else
            widget->setBadge(Badge::Error);  // not yet
    };

    apply(0, ui->w_caller);
    apply(1, ui->w_department);
    apply(2, ui->w_line);
    apply(3, ui->w_employee);
}

void CreateTicketWidget::InitConnections()
{
    // Shared search line edit for all steps.
    connect(ui->le_search, &QLineEdit::textChanged, this,
            [this](const QString& text)
            {
                if (_currentFilterSetter)
                {
                    _currentFilterSetter(text.trimmed());
                }
            });

    connect(ui->tv_lookupTable, &QTableView::doubleClicked, this,
            [this](const QModelIndex& index) { HandleLookupSelection(index); });

    // Enter (Return)
    auto* returnShortcut = new QShortcut(Qt::Key_Return, ui->tv_lookupTable);
    returnShortcut->setContext(Qt::WidgetShortcut);  // Only when table has focus
    connect(returnShortcut, &QShortcut::activated, this,
            [this]()
            {
                const QModelIndex index = ui->tv_lookupTable->currentIndex();
                HandleLookupSelection(index);
            });

    //Enter (Numpad)
    auto* enterShortcut = new QShortcut(Qt::Key_Enter, ui->tv_lookupTable);
    enterShortcut->setContext(Qt::WidgetShortcut);  // Only when table has focus
    connect(enterShortcut, &QShortcut::activated, this,
            [this]()
            {
                const QModelIndex index = ui->tv_lookupTable->currentIndex();
                HandleLookupSelection(index);
            });

    // Enter presst in Lineedit
    connect(ui->le_search, &QLineEdit::returnPressed, this,
            [this]()
            {
                ui->tv_lookupTable->setFocus();

                QAbstractItemModel* model = ui->tv_lookupTable->model();
                if (model == nullptr || model->rowCount() == 0 || model->columnCount() == 0)
                {
                    return;
                }

                const int row = 0;
                const QModelIndex index = model->index(row, 0);

                // Set current index (for keyboard navigation)
                ui->tv_lookupTable->setCurrentIndex(index);

                // Visually select row
                ui->tv_lookupTable->clearSelection();
                ui->tv_lookupTable->selectRow(row);
            });

    auto* searchShortcut = new QShortcut(QKeySequence(Qt::Key_S), ui->tv_lookupTable);
    searchShortcut->setContext(Qt::WidgetShortcut);  // Only when table has focus

    connect(searchShortcut, &QShortcut::activated, this,
            [this]()
            {
                ui->le_search->setFocus();
                ui->le_search->selectAll();
            });
}

void CreateTicketWidget::Initialize()
{
    _currentStep = TicketStep::STEP_1_CALLER;
    _currentModel = nullptr;
    _currentFilterSetter = nullptr;
    _ticketInfo = {};
    _ticketAssignmentInfo = {};

    ui->tv_lookupTable->setModel(nullptr);
    ui->le_search->clear();

    ui->w_caller->setBadge(StatusBadge::Error);
    ui->w_department->setBadge(StatusBadge::Error);
    ui->w_line->setBadge(StatusBadge::Error);
    ui->w_employee->setBadge(StatusBadge::Warning);
    ui->w_employee->setStepColor(Qt::transparent);    

    SetStep(TicketStep::STEP_1_CALLER);

    ui->w_caller->setBadge(StatusBadge::Info);
    ui->w_caller->setStepColor(Qt::transparent);
    ui->w_department->setStepColor(Qt::transparent);
    ui->w_line->setStepColor(Qt::transparent);


    ui->pte_ticketDescription->setEnabled(false);
    ui->le_ticketTitle->setEnabled(false);

    ui->pte_ticketDescription->clear();
    ui->le_ticketTitle->clear();

    ColorizePlainTextEdit(ui->pte_ticketDescription);
    ColorizeLineEdit(ui->le_ticketTitle);

    _saveIsPossible = false;
}

void CreateTicketWidget::ResetColumnHidden() const
{
    QAbstractItemModel* model = ui->tv_lookupTable->model();
    if (!model)
    {
        return;
    }

    const int columnCount = model->columnCount();
    for (int col = 0; col < columnCount; ++col)
    {
        ui->tv_lookupTable->setColumnHidden(col, false);
    }
}


void CreateTicketWidget::LoadCallerTable()
{
    if (!_callerModel)
    {
        _callerModel = new CallerTableModel(this);
        ui->tv_lookupTable->setModel(_callerModel);
        ui->tv_lookupTable->setSortingEnabled(true);
    }

    if (!_callerMgr)
        _callerMgr = std::make_unique<CallerManager>();

    auto task = [this]()
    {
        _callerMgr->LoadCallerData();
        auto localMap = _callerMgr->GetCallerMap();
        _callerTableData = localMap;

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                // Ensure model exists once
                if (!_callerModel)
                {
                    _callerModel = new CallerTableModel(this);
                    ui->tv_lookupTable->setModel(_callerModel);
                    ui->tv_lookupTable->setSortingEnabled(true);
                }

                _callerModel->setData(localMap);

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void CreateTicketWidget::LoadCostUnitTable()
{
    if (!_costUnitModel)
        _costUnitModel = new CostUnitTableModel(this);    

    auto task = [this]()
    {
        auto localMap = CostUnitDataHandler::instance().GetCostUnitMapForLocation();
        _costUnitTableData = localMap;

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {

                if (!_costUnitModel)
                    _costUnitModel = new CostUnitTableModel(this);

                _costUnitModel->setData(localMap);

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void CreateTicketWidget::LoadMachineListTable()
{
    if (!_machineListModel)
        _machineListModel = new MachineListTableModel(this);

    if (!_machineListMgr)
        _machineListMgr = std::make_unique<MachineListManager>();

    auto task = [this]()
    {
        _machineListMgr->LoadMachineListFromDatabase();
        auto localMap = _machineListMgr->GetMachineDataByCostUnitID(_ticketInfo.costUnitID);

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                if (!_machineListModel)
                    _machineListModel = new MachineListTableModel(this);

                _machineListModel->setData(localMap);

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void CreateTicketWidget::LoadEmployeeTable()
{
    if (!_employeeModel)
        _employeeModel = new EmployeeTableModel(this);

    if (!_employeeMgr)
        _employeeMgr = std::make_unique<EmployeeManager>();

    auto task = [this]()
    {
        _employeeMgr->LoadEmployeeData();
        auto localMap = _employeeMgr->GetEmployeeMap();
        _employeeTableData = localMap;

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                if (!_employeeModel)
                    _employeeModel = new EmployeeTableModel(this);

                _employeeModel->setData(localMap);

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void CreateTicketWidget::HandleLookupSelection(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    ResetColumnHidden();

    switch (_currentStep)
    {
        case TicketStep::STEP_1_CALLER:
            SelectCallerFromTable(index);
            SetStep(TicketStep::STEP_2_DEPARTMENT);
            break;

        case TicketStep::STEP_2_DEPARTMENT:
            SelectDepartmentFromTable(index);
            SetStep(TicketStep::STEP_3_LINE_OR_MACHINE);
            break;

        case TicketStep::STEP_3_LINE_OR_MACHINE:
            SelectMachineFromTable(index);
            ui->pte_ticketDescription->setEnabled(true);
            ui->le_ticketTitle->setEnabled(true);
            ColorizePlainTextEdit(ui->pte_ticketDescription, true);
            ColorizeLineEdit(ui->le_ticketTitle, true);
            SetStep(TicketStep::STEP_5_EMPLOYEE);
            _saveIsPossible = true;
            break;

        case TicketStep::STEP_5_EMPLOYEE:
            SelectEmployeeFromTable(index);
            break;

        case TicketStep::STEP_4_INFORMATION:
            break;
    }
}

void CreateTicketWidget::UpdateTableStyle() 
{
    switch (_currentStep)
    {
        case TicketStep::STEP_1_CALLER:
            ui->tv_lookupTable->hideColumn(0);  // ID
            ui->tv_lookupTable->hideColumn(4);  //
            ui->tv_lookupTable->hideColumn(5);  // place
            ui->tv_lookupTable->hideColumn(6);  // is_active
            Util::SetupStretchColumn(ui->tv_lookupTable, 3, 250);
            break;

        case TicketStep::STEP_2_DEPARTMENT:
            ui->tv_lookupTable->hideColumn(0);  // ID
            ui->tv_lookupTable->hideColumn(3);  // Place
            ui->tv_lookupTable->hideColumn(4);  // Barcode
            Util::SetupStretchColumn(ui->tv_lookupTable, 2, 250);
            break;

        case TicketStep::STEP_3_LINE_OR_MACHINE:
            ui->tv_lookupTable->hideColumn(0);   // ID
            ui->tv_lookupTable->hideColumn(1);   // Cost Unit
            ui->tv_lookupTable->hideColumn(10);  // location
            ui->tv_lookupTable->hideColumn(11);  // is_active
            Util::SetupStretchColumn(ui->tv_lookupTable, 5, 250);
            break;

        case TicketStep::STEP_5_EMPLOYEE:
            ui->tv_lookupTable->hideColumn(0);  // ID
            ui->tv_lookupTable->hideColumn(4);  // location
            ui->tv_lookupTable->hideColumn(5);  // is_active
            Util::SetupStretchColumn(ui->tv_lookupTable, 1, 250);
            break;

        case TicketStep::STEP_4_INFORMATION:
            ui->tv_lookupTable->hideColumn(0);  // ID
            ui->tv_lookupTable->hideColumn(4);  // location
            ui->tv_lookupTable->hideColumn(5);  // is_active
            Util::SetupStretchColumn(ui->tv_lookupTable, 1, 250);
            break;
    }
}

void CreateTicketWidget::SelectCallerFromTable(const QModelIndex& index)
{
    if (!index.isValid() || !_callerModel)
        return;

    auto idOpt = Util::GetCallerIdFromIndex(ui->tv_lookupTable, index);
    if (!idOpt.has_value())
        return;

    _ticketInfo.reporterID = idOpt.value();

    auto data = _callerTableData.find(_ticketInfo.reporterID);

    if (data != _callerTableData.end())
    {
        QString callerData = QString::fromStdString(data->second.phone) + " " + QString::fromStdString(data->second.name);
        ui->lb_selectedCaller->setText(callerData);
    }
}

void CreateTicketWidget::SelectDepartmentFromTable(const QModelIndex& index)
{
    if (!index.isValid() || !_costUnitModel)
        return;
    
    auto idOpt = Util::GetIdFromIndex(ui->tv_lookupTable, index);

    if (!idOpt.has_value())
        return;

    _ticketInfo.costUnitID = static_cast<std::uint16_t>(idOpt.value());

    auto costUnit = CostUnitDataHandler::instance().GetCostUnitNameByInternalId(_ticketInfo.costUnitID);
    ui->lb_selectedDepartment->setText(QString::fromStdString(costUnit));
}

void CreateTicketWidget::SelectMachineFromTable(const QModelIndex& index)
{
    if (!index.isValid() || !_machineListModel)
        return;

    auto idOpt = Util::GetIdFromIndex(ui->tv_lookupTable, index);

    if (!idOpt.has_value())
        return;

    _ticketInfo.entityID = static_cast<std::uint16_t>(idOpt.value());

    if (auto machineInfo = _machineListMgr->GetById(_ticketInfo.entityID))
    {
        ui->lb_selectedLine->setText(QString::fromStdString(machineInfo->MachineName));
    }
}

void CreateTicketWidget::SelectEmployeeFromTable(const QModelIndex& index)
{
    if (!index.isValid() || !_employeeModel)
        return;

    auto idOpt = Util::GetIdFromIndex(ui->tv_lookupTable, index);

    if (!idOpt.has_value())
        return;

    _ticketAssignmentInfo.employeeID = static_cast<std::uint32_t>(idOpt.value());

    const auto it = _employeeTableData.find(static_cast<std::uint32_t>(idOpt.value()));
    if (it == _employeeTableData.end())
        return;

    const auto& employeeInfo = it->second;

    QString employeeData = QString::fromStdString(employeeInfo.phone) + " " + QString::fromStdString(employeeInfo.lastName);
    ui->lb_selectedEmployee->setText(employeeData);
}

void CreateTicketWidget::ColorizePlainTextEdit(QPlainTextEdit* pte, bool reset /*= false*/)
{
    if (reset)
    {
        pte->setPalette(QPalette());
        return;
    }

    QPalette pal = pte->palette();
    pal.setColor(QPalette::Text, QColor("#B00000"));
    pal.setColor(QPalette::Base, QColor("#FFE5E5"));
    pte->setPalette(pal);
}

void CreateTicketWidget::ColorizeLineEdit(QLineEdit* pte, bool reset /*= false*/)
{
    if (reset)
    {
        pte->setPalette(QPalette());
        return;
    }

    QPalette pal = pte->palette();
    pal.setColor(QPalette::Text, QColor("#B00000"));
    pal.setColor(QPalette::Base, QColor("#FFE5E5"));
    pte->setPalette(pal);
}

bool CreateTicketWidget::CheckCanSave()
{
    if (!_saveIsPossible)
    {
        MessageBoxHelper::ShowInfoMessage(tr("You cannot save until all steps are completed."));
        return false;
    }

    if (ui->le_ticketTitle->text().isEmpty())
    {
        MessageBoxHelper::ShowInfoMessage(tr("Ticket title cannot be empty."));
        return false;
    }

    if (ui->pte_ticketDescription->toPlainText().isEmpty())
    {
        MessageBoxHelper::ShowInfoMessage(tr("Ticket description cannot be empty."));
        return false;
    }

    return true;
}

void CreateTicketWidget::onPushReset()
{
    Initialize();
    ui->lb_selectedEmployee->clear();
    ui->lb_selectedLine->clear();
    ui->lb_selectedDepartment->clear();
    ui->lb_selectedCaller->clear();
    ui->pte_ticketDescription->clear();
}

void CreateTicketWidget::onPushSave()
{
    if (!CheckCanSave())
        return;

    if (!_createTicketMgr)
        _createTicketMgr = std::make_unique<CreateTicketManager>();

    _ticketInfo.title = ui->le_ticketTitle->text().toStdString();
    _ticketInfo.description = ui->pte_ticketDescription->toPlainText().toStdString();
    _ticketInfo.currentStatus = static_cast<std::uint8_t>(TicketStatus::TICKET_STATUS_NEW);
    _ticketInfo.priority = static_cast<std::uint8_t>(TicketPriority::TICKET_PRIORITY_TRIVIAL);

    if (_createTicketMgr->SaveTicket(_ticketInfo, _ticketAssignmentInfo))
        GlobalSignals::instance()->SendStatusMessage(tr("Ticket saved Succesfully").toStdString(), StatusMessageQueue::QUEUE, Qt::darkGreen);
    else
        GlobalSignals::instance()->SendStatusMessage(tr("Ticket creation failed").toStdString(), StatusMessageQueue::INSTANT, Qt::red);

    onPushReset();
    ui->le_search->setFocus();

    if (_isExternal)
    {
        onPushReset();
        ui->le_search->setFocus();
    }
    else
        emit GlobalSignals::instance()->CreateTicketBackToMainPage();
}

void CreateTicketWidget::onEscPressed()
{
    switch (_currentStep)
    {
        case TicketStep::STEP_1_CALLER:
            if (_isExternal)
            {
                onPushReset();
            }
            else
                emit GlobalSignals::instance()->CreateTicketBackToMainPage();
            break;

        case TicketStep::STEP_2_DEPARTMENT:
            SetStep(TicketStep::STEP_1_CALLER);
            break;

        case TicketStep::STEP_3_LINE_OR_MACHINE:
            SetStep(TicketStep::STEP_2_DEPARTMENT);
            break;

        case TicketStep::STEP_5_EMPLOYEE:
            SetStep(TicketStep::STEP_3_LINE_OR_MACHINE);
            break;

        default:
            break;
    }
}

