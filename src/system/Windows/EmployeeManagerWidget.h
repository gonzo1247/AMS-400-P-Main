#pragma once

#include <QWidget>

#include "DatabaseDefines.h"
#include "EmployeeManager.h"
#include "EmployeeTableModel.h"
#include "ui_EmployeeManagerWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class EmployeeManagerWidgetClass; };
QT_END_NAMESPACE

enum EmployeeManagerWidgetPages
{
	EMPLOYEE_MANAGER_WIDGET_PAGE_EMPLOYEE_TABLE		= 0,
	EMPLOYEE_MANAGER_WIDGET_PAGE_ADD_EDIT_EMPLOYEE	= 1
};

class EmployeeManagerWidget : public QWidget
{
	Q_OBJECT

public:
	EmployeeManagerWidget(QWidget *parent = nullptr);
	~EmployeeManagerWidget();

	void LoadEmployeeData();

private:
	EmployeeInformation FetchEmployeeData();
    bool CheckEmployeeData();
	void AddNewEmployee(const EmployeeInformation& info);
	void ClearFields();
	void ReloadTable();
    void SetupTable();

	Ui::EmployeeManagerWidgetClass *ui;
    EmployeeTableModel* _employeeModel;
    std::unique_ptr<EmployeeManager> _employeeMgr;

	std::uint32_t _editEmployeeID;

	bool _isEmployeeEdit;
	bool _tableIsReloaded;

	std::unordered_map<std::uint32_t, EmployeeInformation> _employeeMap;

private slots:
	void onPushNewEmployee();
	void onPushBack();
    void onPushSaveEmployee();
	void onPushEditEmployee();
	void onPushDeleteEmployee();
};

