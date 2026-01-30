#include "EmployeeManagerWidget.h"

#include "CompanyLocationHandler.h"
#include "MessageBoxHelper.h"
#include "SettingsManager.h"
#include "TranslateMessageHelper.h"

EmployeeManagerWidget::EmployeeManagerWidget(QWidget *parent) : QWidget(parent), ui(new Ui::EmployeeManagerWidgetClass()), _employeeMgr(nullptr), _isEmployeeEdit(false), _tableIsReloaded(false)
{
	ui->setupUi(this);
    _employeeModel = new EmployeeTableModel(this);
    ui->tv_employee->setModel(_employeeModel);
    ui->tv_employee->setSortingEnabled(true);
    LoadEmployeeData();
    CompanyLocationHandler::instance().FillComboBoxWithData(ui->cb_p1_companyLoc);
    CompanyLocationHandler::instance().FillComboBoxWithData(ui->cb_p2_companyLoc);

    {
        auto updateFilter = [this]()
        {
            QString filter;

            if (!ui->le_p1_firstName->text().trimmed().isEmpty())
                filter += ui->le_p1_firstName->text().trimmed() + " ";

            if (!ui->le_p1_lastName->text().trimmed().isEmpty())
                filter += ui->le_p1_lastName->text().trimmed() + " ";

            if (!ui->le_p1_phone->text().trimmed().isEmpty())
                filter += ui->le_p1_phone->text().trimmed() + " ";

            filter = filter.trimmed();

            // Checkbox: "Include Deleted Caller"
            const bool includeDeleted = ui->chb_p1_deletedEmployee->isChecked();
            const bool onlyActive = !includeDeleted;

            _employeeModel->setFilterText(filter);
            _employeeModel->setOnlyActive(onlyActive);
        };

        connect(ui->le_p1_firstName, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->le_p1_lastName, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->le_p1_phone, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->chb_p1_deletedEmployee, &QCheckBox::toggled, this, [updateFilter](bool) { updateFilter(); });

        connect(ui->pb_p1_reset, &QPushButton::clicked, this,
                [this]()
                {
                    ui->le_p1_firstName->clear();
                    ui->le_p1_lastName->clear();
                    ui->le_p1_phone->clear();
                    ui->chb_p1_deletedEmployee->setChecked(false);

                    _employeeModel->setFilterText(QString());
                    _employeeModel->setOnlyActive(true);
                });
    }

    // Button connections
	{
	    connect(ui->pb_p1_add, &QPushButton::clicked, this, &EmployeeManagerWidget::onPushNewEmployee);
	    connect(ui->pb_p2_back, &QPushButton::clicked, this, &EmployeeManagerWidget::onPushBack);
        connect(ui->pb_p2_save, &QPushButton::clicked, this, &EmployeeManagerWidget::onPushSaveEmployee);
        connect(ui->pb_p1_edit, &QPushButton::clicked, this, &EmployeeManagerWidget::onPushEditEmployee);
        connect(ui->pb_p1_delete, &QPushButton::clicked, this, &EmployeeManagerWidget::onPushDeleteEmployee);
	}

    ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_EMPLOYEE_TABLE);
}

EmployeeManagerWidget::~EmployeeManagerWidget()
{
	delete ui;
}

void EmployeeManagerWidget::LoadEmployeeData()
{
    if (!_employeeMgr)
        _employeeMgr = std::make_unique<EmployeeManager>();

    auto task = [this]()
    {
        _employeeMgr->LoadEmployeeData();
        _employeeMap = _employeeMgr->GetEmployeeMap();
        auto localMap = _employeeMgr->GetEmployeeMap();

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                // Ensure model exists once
                if (!_employeeModel)
                {
                    _employeeModel = new EmployeeTableModel(this);
                    ui->tv_employee->setModel(_employeeModel);
                    ui->tv_employee->setSortingEnabled(true);
                }

                _employeeModel->setData(localMap);
                SetupTable();
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

EmployeeInformation EmployeeManagerWidget::FetchEmployeeData()
{
    EmployeeInformation info;
    info.firstName = ui->le_p2_firstName->text().trimmed().toStdString();
    info.lastName = ui->le_p2_lastName->text().trimmed().toStdString();
    info.phone = ui->le_p2_phone->text().trimmed().toStdString();
    info.location = Util::ComboGetValue<std::uint32_t>(ui->cb_p2_companyLoc);
    info.isActive = true;

    return info;
}

bool EmployeeManagerWidget::CheckEmployeeData()
{
    if (ui->le_p2_lastName->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, tr("Input Error"), tr("Last name cannot be empty."));
        return false;
    }

    if (ui->le_p2_phone->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, tr("Input Error"), tr("Phone number cannot be empty."));
        return false;
    }

    return true;
}

void EmployeeManagerWidget::AddNewEmployee(const EmployeeInformation& info)
{
    if (!_isEmployeeEdit)
    {
        if (_employeeMgr->AddNewEmployee(info))
        {
            MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_ADDED);
            LoadEmployeeData();
            ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_EMPLOYEE_TABLE);
            ClearFields();
        }
        else
        {
            MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_EMPLOYEE_ADD_NOT_WORKING);
        }

        return;
    }

    EmployeeInformation employeeInfo = FetchEmployeeData();
    employeeInfo.id = _editEmployeeID;

    if (_employeeMgr->UpdateEmployee(employeeInfo))
    {
        MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_UPDATED);
        LoadEmployeeData();
        ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_EMPLOYEE_TABLE);
        _isEmployeeEdit = false;
        _editEmployeeID = 0;
        ClearFields();
    }
    else
    {
        MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_EMPLOYEE_UPDATE_NOT_WORKING);
    }
}

void EmployeeManagerWidget::ClearFields()
{
    ui->le_p2_firstName->clear();
    ui->le_p2_lastName->clear();
    ui->le_p2_phone->clear();
}

void EmployeeManagerWidget::ReloadTable()
{
    if (!_tableIsReloaded)
    {
        if (!_employeeMgr)
            _employeeMgr = std::make_unique<EmployeeManager>();

        auto task = [this]()
        {
            _employeeMap = _employeeMgr->GetEmployeeMap();
            auto localMap = _employeeMgr->GetEmployeeMap();

            QMetaObject::invokeMethod(
                this,
                [this, localMap = std::move(localMap)]()
                {
                    // Ensure model exists once
                    if (!_employeeModel)
                    {
                        _employeeModel = new EmployeeTableModel(this);
                        ui->tv_employee->setModel(_employeeModel);
                        ui->tv_employee->setSortingEnabled(true);
                    }

                    _employeeModel->setData(localMap);
                    SetupTable();
                },
                Qt::QueuedConnection);
        };

        Util::RunInThread(std::move(task), this);
        _tableIsReloaded = true;
    }

    QTimer::singleShot(1000, this, [this]() { _tableIsReloaded = false; });
}

void EmployeeManagerWidget::SetupTable()
{
    auto* header = ui->tv_employee->horizontalHeader();

    ui->tv_employee->setWordWrap(false);
    ui->tv_employee->setTextElideMode(Qt::ElideRight);

    header->setHighlightSections(false);
    header->setStretchLastSection(false);

    header->setSectionResizeMode(QHeaderView::Interactive);

    // Keep small columns stable
    header->setSectionResizeMode(0, QHeaderView::Fixed);             // ID
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Phone
    header->setSectionResizeMode(5, QHeaderView::Fixed);             // Active

    // Use remaining space for names
    header->setSectionResizeMode(1, QHeaderView::Stretch);  // First name
    header->setSectionResizeMode(2, QHeaderView::Stretch);  // Last name

    // Location usually needs some room but not all
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    ui->tv_employee->setColumnWidth(0, 55);
    ui->tv_employee->setColumnWidth(5, 80);
    ui->tv_employee->setColumnWidth(4, 160);

    // Optional: avoid horizontal scrollbar jumps
    ui->tv_employee->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->tv_employee->hideColumn(0);  // Hide ID column

    ui->tv_employee->sortByColumn(2, Qt::AscendingOrder);  // Name
}

void EmployeeManagerWidget::onPushNewEmployee()
{
    ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_ADD_EDIT_EMPLOYEE);
}

void EmployeeManagerWidget::onPushBack()
{
    ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_EMPLOYEE_TABLE);
}

void EmployeeManagerWidget::onPushSaveEmployee()
{
    if (!CheckEmployeeData())
        return;
    
    EmployeeInformation info = FetchEmployeeData();
    AddNewEmployee(info);
}

void EmployeeManagerWidget::onPushEditEmployee()
{
    _isEmployeeEdit = true;

    auto employeeID = Util::GetSelectedCallerId(ui->tv_employee);

    if (!employeeID.has_value())
    {
        LOG_WARNING("EmployeeManagerWidget::onPushEditEmployee: No employee selected.");
        QMessageBox::warning(this, tr("Edit Employee"), tr("Please select an employee entry first."));
        return;
    }

    EmployeeInformation info = _employeeMap[static_cast<std::uint32_t>(employeeID.value())];
    ui->le_p2_firstName->setText(QString::fromStdString(info.firstName));
    ui->le_p2_lastName->setText(QString::fromStdString(info.lastName));
    ui->le_p2_phone->setText(QString::fromStdString(info.phone));
    auto locIndex = ui->cb_p2_companyLoc->findData(QVariant(static_cast<int>(info.location)));
    if (locIndex != -1)
        ui->cb_p2_companyLoc->setCurrentIndex(locIndex);

    _editEmployeeID = static_cast<std::uint32_t>(employeeID.value());

    ui->stackedWidget->setCurrentIndex(EMPLOYEE_MANAGER_WIDGET_PAGE_ADD_EDIT_EMPLOYEE);
}

void EmployeeManagerWidget::onPushDeleteEmployee()
{
    auto employeeID = Util::GetSelectedCallerId(ui->tv_employee);

    if (!employeeID.has_value())
    {
        LOG_WARNING("EmployeeManagerWidget::onPushEditEmployee: No employee selected.");
        QMessageBox::warning(this, tr("Edit Employee"), tr("Please select an employee entry first."));
        return;
    }

    auto info = _employeeMap.find(*employeeID);
    if (info == _employeeMap.end())
    {
        LOG_ERROR("CallerManagerWidget::onPushDeleteCaller: Caller ID {} not found in employee map.", *employeeID);
        QMessageBox::warning(this, tr("Delete Caller"), tr("Selected caller entry not found."));
        return;
    }

    
    const auto result =
        QMessageBox::question(this, tr("Delete Caller"),
                              TranslateMessageHelper::LoadErrorCodeMessage(ErrorCodes::WARNING_AMS_SURE_DELETE_CALLER)
                                  .arg(QString::fromUtf8(info->second.firstName + " " + info->second.lastName)),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);  // default button

    if (result != QMessageBox::Yes)
        return;

    if (_employeeMgr->DeleteEmployee(static_cast<std::uint32_t>(*employeeID)))
    {
        MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_DELETED);
        LoadEmployeeData();
    }
    else
        MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_EMPLOYEE_DELETE_NOT_WORKING);
}

