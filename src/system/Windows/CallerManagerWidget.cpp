#include "CallerManagerWidget.h"

#include <CompanyLocationHandler.h>

#include <QScrollBar>

#include "CallerTableModel.h"
#include "CostUnitDataHandler.h"
#include "GlobalSignals.h"
#include "MessageBoxHelper.h"
#include "SettingsManager.h"
#include "TranslateMessageHelper.h"
#include "Util.h"

CallerManagerWidget::CallerManagerWidget(QWidget* parent) : QWidget(parent), ui(new Ui::CallerManagerWidgetClass()), _callerMgr(nullptr), _tableCanReloaded(true), _callerEditMode(false), _callerEditID(0)
{
	ui->setupUi(this);

	_callerModel = new CallerTableModel(this);
    ui->tv_callerTable->setModel(_callerModel);
    ui->tv_callerTable->setSortingEnabled(true);
    LoadCallerInformation();
    CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_searchCostUnit, GetSettings().getLanguage(), GetCompanyLocationsString(GetSettings().getCompanyLocation()));
	CompanyLocationHandler::instance().FillComboBoxWithData(ui->cb_companyLocation);

    ui->stackedWidget->setCurrentIndex(0);

    // Add or Edit Site
	{
        CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_p2_addCostUnit, GetSettings().getLanguage(),GetCompanyLocationsString(GetSettings().getCompanyLocation()));
        CompanyLocationHandler::instance().FillComboBoxWithData(ui->cb_p2_addCompanyLocation);
	}
	
	{
        auto index = ui->cb_companyLocation->findData(QVariant(static_cast<int>(GetSettings().getCompanyLocation())));
		if (index != -1)
            ui->cb_companyLocation->setCurrentIndex(index);
	}

	{
        auto updateFilter = [this]()
        {
            QString filter;

            if (!ui->le_searchName->text().trimmed().isEmpty())
                filter += ui->le_searchName->text().trimmed() + " ";

            if (!ui->le_searchPhone->text().trimmed().isEmpty())
                filter += ui->le_searchPhone->text().trimmed() + " ";

            if (!ui->le_searchDepartmnet->text().trimmed().isEmpty())
                filter += ui->le_searchDepartmnet->text().trimmed() + " ";

            filter = filter.trimmed();

            // Checkbox: "Include Deleted Caller"
            const bool includeDeleted = ui->chb_searchDeletedCaller->isChecked();
            const bool onlyActive = !includeDeleted;

            _callerModel->setFilterText(filter);
            _callerModel->setOnlyActive(onlyActive);
        };

        connect(ui->le_searchName, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->le_searchPhone, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->le_searchDepartmnet, &QLineEdit::textChanged, this, [updateFilter]() { updateFilter(); });
        connect(ui->chb_searchDeletedCaller, &QCheckBox::toggled, this, [updateFilter](bool) { updateFilter(); });

        connect(ui->pb_reset, &QPushButton::clicked, this, [this]()
        {
            ui->le_searchName->clear();
            ui->le_searchPhone->clear();
            ui->le_searchDepartmnet->clear();
            ui->chb_searchDeletedCaller->setChecked(false);

            _callerModel->setFilterText(QString());
            _callerModel->setOnlyActive(true);
        });
	}

    // Buttons
	{
	    connect(ui->pb_addNewCaller, &QPushButton::clicked, this, &CallerManagerWidget::onPushNewCaller);
        connect(ui->pb_p2_back, &QPushButton::clicked, this, &CallerManagerWidget::onPushBack);
        connect(ui->pb_p2_addNew, &QPushButton::clicked, this, &CallerManagerWidget::onPushSaveCaller);
        connect(ui->pb_editCaller, &QPushButton::clicked, this, &CallerManagerWidget::onPushEditCaller);
        connect(ui->pb_deleteCaller, &QPushButton::clicked, this, &CallerManagerWidget::onPushDeleteCaller);
	}

    // Signals
	{	    
	  connect(GlobalSignals::instance(), &GlobalSignals::SignalReloadCallerTable, this, &CallerManagerWidget::onReceiveTableReloadSignal);
    }

    // Cost Unit ComboBox -> Change Cost unit if company location changes
	{
	    connect(ui->cb_companyLocation, &QComboBox::currentIndexChanged, this, &CallerManagerWidget::onCompanyLocationChanged);
	    connect(ui->cb_p2_addCompanyLocation, &QComboBox::currentIndexChanged, this, &CallerManagerWidget::onCompanyLocationChangedSearch);
	}
}

CallerManagerWidget::~CallerManagerWidget()
{
	delete ui;
}

void CallerManagerWidget::LoadCallerInformation()
{
    if (!_callerMgr)
        _callerMgr = std::make_unique<CallerManager>();


	auto task = [this]()
	{
	    _callerMgr->LoadCallerData();
        callerMap = _callerMgr->GetCallerMap();
		auto localMap = _callerMgr->GetCallerMap();

		QMetaObject::invokeMethod(this, [this, localMap = std::move(localMap)]()
		{
            // Ensure model exists once
            if (!_callerModel)
            {
                _callerModel = new CallerTableModel(this);
                ui->tv_callerTable->setModel(_callerModel);
                ui->tv_callerTable->setSortingEnabled(true);
				CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_searchCostUnit, GetSettings().getLanguage(), GetCompanyLocationsString(GetSettings().getCompanyLocation()));
            }

			_callerModel->setData(localMap);
            SetupTable();

		}, Qt::QueuedConnection);
	};

	Util::RunInThread(std::move(task), this);
}

CallerInformation CallerManagerWidget::FetchCallerInformation()
{
    CallerInformation info;
    info.department = ui->le_p2_addDepartment->text().toStdString();
    info.phone = ui->le_p2_addPhone->text().toStdString();
    info.name = ui->le_p2_addName->text().toStdString();
    info.costUnitID = ui->cb_p2_addCostUnit->currentData().toUInt();
    info.companyLocationID = ui->cb_p2_addCompanyLocation->currentData().toUInt();
    info.is_active = true;
    info.id = 0;

    return info;
}

bool CallerManagerWidget::CheckCallerInformation()
{
    // Phone, Name and location must be filled

    if (ui->le_p2_addPhone->text().trimmed().isEmpty())
    {
        LOG_WARNING("CallerManagerWidget::CheckCallerInformation: Phone field is empty.");
        return false;
    }

    if (ui->le_p2_addName->text().trimmed().isEmpty())
    {
        LOG_WARNING("CallerManagerWidget::CheckCallerInformation: Name field is empty.");
        return false;
    }

    if (ui->cb_p2_addCompanyLocation->currentIndex() == -1 || ui->cb_p2_addCompanyLocation->currentIndex() == 0)
    {
        LOG_WARNING("CallerManagerWidget::CheckCallerInformation: Company location is not selected.");
        return false;
    }

    return true;
}

void CallerManagerWidget::ClearFields()
{
    ui->cb_p2_addCompanyLocation->setCurrentIndex(1);
    ui->le_p2_addName->clear();
    ui->le_p2_addPhone->clear();
    ui->le_p2_addDepartment->clear();
    ui->cb_p2_addCostUnit->setCurrentIndex(0);
    _callerEditID = 0;
    _callerEditMode = false;
}

void CallerManagerWidget::LoadEditCallerDataToUI(std::uint64_t callerID)
{
    auto data = callerMap.find(callerID);
    CallerInformation editData;
    if (data != callerMap.end())
    {
        editData = data->second;
    }
    else
    {
        LOG_ERROR("CallerManagerWidget::LoadEditCallerDataToUI: Caller ID {} not found in caller map.", callerID);
        return;
    }

    ui->le_p2_addDepartment->setText(QString::fromStdString(editData.department));
    ui->le_p2_addName->setText(QString::fromStdString(editData.name));
    ui->le_p2_addPhone->setText(QString::fromStdString(editData.phone));

    auto locIndex = ui->cb_p2_addCompanyLocation->findData(QVariant(static_cast<int>(editData.companyLocationID)));
    if (locIndex != -1)
        ui->cb_p2_addCompanyLocation->setCurrentIndex(locIndex);

    auto costIndex = ui->cb_p2_addCostUnit->findData(QVariant(static_cast<int>(editData.costUnitID)));
    if (costIndex != -1)
        ui->cb_p2_addCostUnit->setCurrentIndex(costIndex);

}

void CallerManagerWidget::ReloadCallerTable()
{
    if (!_callerMgr)
        _callerMgr = std::make_unique<CallerManager>();

    auto task = [this]()
    {
        callerMap = _callerMgr->GetCallerMap();
        auto localMap = _callerMgr->GetCallerMap();

        QMetaObject::invokeMethod(
            this,
            [this, localMap = std::move(localMap)]()
            {
                // Ensure model exists once
                if (!_callerModel)
                {
                    _callerModel = new CallerTableModel(this);
                    ui->tv_callerTable->setModel(_callerModel);
                    ui->tv_callerTable->setSortingEnabled(true);
                    CostUnitDataHandler::instance().FillComboBoxWithData(
                        ui->cb_searchCostUnit, GetSettings().getLanguage(),
                        GetCompanyLocationsString(GetSettings().getCompanyLocation()));
                }

                _callerModel->setData(localMap);
                SetupTable();
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void CallerManagerWidget::SetupTable()
{
    ui->tv_callerTable->hideColumn(0);  // internal ID

    auto* header = ui->tv_callerTable->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // ID
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Department
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Phone
    header->setSectionResizeMode(3, QHeaderView::Stretch);           // Name
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Cost Unit
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);  // Location
    header->setSectionResizeMode(6, QHeaderView::Fixed);             // Active

    ui->tv_callerTable->setColumnWidth(0, 60);  // ID
    ui->tv_callerTable->setColumnWidth(2, 90);  // Phone
    ui->tv_callerTable->setColumnWidth(6, 70);  // Active

    header->setStretchLastSection(false);

    ui->tv_callerTable->sortByColumn(3, Qt::AscendingOrder);  // Name
    ui->tv_callerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void CallerManagerWidget::onPushNewCaller()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void CallerManagerWidget::onPushBack()
{
    ui->stackedWidget->setCurrentIndex(0);
    _callerEditMode = false;
    _callerEditID = 0;
}

void CallerManagerWidget::onPushSaveCaller()
{
    if (!CheckCallerInformation())
    {
        LOG_WARNING("CallerManagerWidget::onPushSaveCaller: Caller information validation failed.");
        MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_MISSING_REQUIRED_FIELDS, true);
        return;
    }

    CallerInformation info = FetchCallerInformation();

    if (!_callerEditMode)
    {
        if (_callerMgr->AddCaller(info))
        {
            LOG_DEBUG("CallerManagerWidget::onPushSaveCaller: Caller '{}' added successfully.", info.name);
            MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_CALLER_SUCCESSFULLY_ADDED, true);
            ui->stackedWidget->setCurrentIndex(0);
            ClearFields();
        }
        else
        {
            LOG_ERROR("CallerManagerWidget::onPushSaveCaller: Failed to add caller '{}'.", info.name);
            MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_CALLER_NOT_ADDED);
        }
    }
    else
    {
        info.id = _callerEditID;
        if (_callerMgr->EditCaller(info))
        {
            
            LOG_DEBUG("CallerManagerWidget::onPushSaveCaller: Caller '{}' updated successfully.", info.name);
            MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_CALLER_SUCCESSFULLY_UPDATED, true);
            ui->stackedWidget->setCurrentIndex(0);
            _callerEditMode = false;
            _callerEditID = 0;
            ClearFields();
        }
        else
        {            
            LOG_WARNING("CallerManagerWidget::onPushSaveCaller: No caller details to update.");
            MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_CALLER_NOT_ADDED);
        }
    }

}

void CallerManagerWidget::onReceiveTableReloadSignal()
{
    if (_tableCanReloaded)
    {
        ReloadCallerTable();
        _tableCanReloaded = false;
    }

    QTimer::singleShot(2000, this, [&]() { _tableCanReloaded = true; });
}

void CallerManagerWidget::onPushEditCaller()
{
    auto callerID = Util::GetSelectedCallerId(ui->tv_callerTable);

    if (!callerID.has_value())
    {
        LOG_WARNING("CallerManagerWidget::onPushEditCaller: No caller selected.");
        QMessageBox::warning(this, tr("Edit Caller"), tr("Please select a caller entry first."));
        return;
    }

    LoadEditCallerDataToUI(*callerID);
    ui->stackedWidget->setCurrentIndex(1);
    _callerEditMode = true;
    _callerEditID = *callerID;

}

void CallerManagerWidget::onPushDeleteCaller()
{
    auto callerID = Util::GetSelectedCallerId(ui->tv_callerTable);

    if (!callerID.has_value())
    {
        LOG_WARNING("CallerManagerWidget::onPushEditCaller: No caller selected.");
        QMessageBox::warning(this, tr("Edit Caller"), tr("Please select a caller entry first."));
        return;
    }

    auto info = callerMap.find(*callerID);
    if (info == callerMap.end())
    {
        LOG_ERROR("CallerManagerWidget::onPushDeleteCaller: Caller ID {} not found in caller map.", *callerID);
        QMessageBox::warning(this, tr("Delete Caller"), tr("Selected caller entry not found."));
        return;
    }

    const auto result = QMessageBox::question(this, tr("Delete Caller"), 
    TranslateMessageHelper::LoadErrorCodeMessage(ErrorCodes::WARNING_AMS_SURE_DELETE_CALLER).arg(QString::fromUtf8(info->second.name)),
    QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);  // default button

    if (result != QMessageBox::Yes)
    {
        return;
    }

    if (_callerMgr->RemoveCaller(*callerID))
        MessageBoxHelper::ShowSuccessfullyMessage(SuccessfullCodes::MESSAGE_AMS_CALLER_SUCCESSFULLY_DELETED);
    else
        MessageBoxHelper::ShowWarningMessage(ErrorCodes::WARNING_AMS_CALLER_DELETE_NOT_WORKING);
}

void CallerManagerWidget::onCompanyLocationChanged(int /*index*/)
{
    int id = Util::ComboGetValue<int>(ui->cb_companyLocation);
    CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_searchCostUnit, GetSettings().getLanguage(), GetCompanyLocationsString(static_cast<CompanyLocations>(id)));
}

void CallerManagerWidget::onCompanyLocationChangedSearch(int index)
{
    int id = Util::ComboGetValue<int>(ui->cb_p2_addCompanyLocation);
    CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_p2_addCostUnit, GetSettings().getLanguage(), GetCompanyLocationsString(static_cast<CompanyLocations>(id)));
}

