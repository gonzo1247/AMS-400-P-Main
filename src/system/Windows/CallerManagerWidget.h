#pragma once

#include <QWidget>

#include "CallerManager.h"
#include "CallerTableModel.h"
#include "DatabaseDefines.h"
#include "ui_CallerManagerWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CallerManagerWidgetClass; };
QT_END_NAMESPACE

class CallerManagerWidget : public QWidget
{
	Q_OBJECT

public:
	CallerManagerWidget(QWidget *parent = nullptr);
	~CallerManagerWidget();

	void setExternal() { _isExternal = true; }

private:
    void LoadCallerInformation();
	CallerInformation FetchCallerInformation();
	bool CheckCallerInformation();
	void ClearFields();
	void LoadEditCallerDataToUI(std::uint64_t callerID);
	void ReloadCallerTable();
    void SetupTable();

	Ui::CallerManagerWidgetClass *ui;
	std::unique_ptr<CallerManager> _callerMgr;
	CallerTableModel* _callerModel;

	bool _tableCanReloaded;
	bool _callerEditMode;
	bool _isExternal = false;

	std::uint64_t _callerEditID;


	std::unordered_map<std::uint64_t, CallerInformation> callerMap;

private slots:
	void onPushNewCaller();
	void onPushBack();
	void onPushSaveCaller();
	void onReceiveTableReloadSignal();
	void onPushEditCaller();
	void onPushDeleteCaller();
    void onCompanyLocationChanged(int index);
    void onCompanyLocationChangedSearch(int index);
};

