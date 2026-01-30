#pragma once

#include <QWidget>

#include "CallerManager.h"
#include "CallerTableModel.h"
#include "CostUnitTableModel.h"
#include "CreateTicketManager.h"
#include "EmployeeManager.h"
#include "EmployeeTableModel.h"
#include "MachineListManager.h"
#include "MachineListTableModel.h"
#include "ui_CreateTicketWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CreateTicketWidgetClass; };
QT_END_NAMESPACE

enum class TicketStep : std::uint8_t
{
	STEP_1_CALLER			= 0,
	STEP_2_DEPARTMENT		= 1,
	STEP_3_LINE_OR_MACHINE	= 2,
	STEP_4_INFORMATION		= 3,
    STEP_5_EMPLOYEE			= 4
};

class CreateTicketWidget : public QWidget
{
	Q_OBJECT

public:
	CreateTicketWidget(QWidget *parent = nullptr);
	~CreateTicketWidget();

	void setModels(CallerTableModel* callerModel, CostUnitTableModel* costUnitModel, MachineListTableModel* _machineListModel, EmployeeTableModel* employeeModel);
    void SetExternal() { _isExternal = true; }

private:
	void InitSteps();
	void SetStep(TicketStep step);
	void UpdateStepUI();
	void InitConnections();

	void Initialize();

	void ResetColumnHidden() const;

	void LoadCallerTable();
	void LoadCostUnitTable();
	void LoadMachineListTable();
	void LoadEmployeeTable();
    void HandleLookupSelection(const QModelIndex& index);

	//Table Helpers to Style the diffrent Steps
    void UpdateTableStyle();

	// Table Helpers to get the data
	void SelectCallerFromTable(const QModelIndex& index);
    void SelectDepartmentFromTable(const QModelIndex& index);
    void SelectMachineFromTable(const QModelIndex& index);
    void SelectEmployeeFromTable(const QModelIndex& index);

	void ColorizePlainTextEdit(QPlainTextEdit* pte, bool reset = false);
	void ColorizeLineEdit(QLineEdit* pte, bool reset = false);

	bool CheckCanSave();

	Ui::CreateTicketWidgetClass *ui;
	
	TicketStep _currentStep { TicketStep::STEP_1_CALLER };

	CallerTableModel* _callerModel;
    CostUnitTableModel* _costUnitModel;
	MachineListTableModel* _machineListModel;
    EmployeeTableModel* _employeeModel;

	std::unique_ptr<CallerManager> _callerMgr;
    std::unique_ptr<EmployeeManager> _employeeMgr;
    std::unique_ptr<MachineListManager> _machineListMgr;
	std::unique_ptr<CreateTicketManager> _createTicketMgr;

	QAbstractItemModel* _currentModel{ nullptr };

	std::function<void(const QString&)> _currentFilterSetter;

	TicketInformation _ticketInfo{};
	TicketAssignmentInformation _ticketAssignmentInfo{};

	// Table data
	std::unordered_map<std::uint64_t, CallerInformation> _callerTableData;
    std::unordered_map<std::uint16_t, CostUnitInformation> _costUnitTableData;
	std::unordered_map<std::uint32_t, EmployeeInformation> _employeeTableData;

	bool _saveIsPossible;
	bool _isExternal = false;

private slots:
	void onPushReset();
	void onPushSave();
	void onEscPressed();

};
