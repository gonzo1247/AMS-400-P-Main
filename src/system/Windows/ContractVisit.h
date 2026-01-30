#pragma once

#include <QWidget>
#include <QPointer>

#include <unordered_set>

#include "ContractorVisitManager.h"
#include "ContractorVisitModel.h"
#include "ContractorVisitProxyModel.h"
#include "ContractorVisitWorkerModel.h"
#include "ui_ContractVisit.h"

class ContractorVisitStatusModel;
class CompanyContactData;
class ContractorWorkerData;
QT_BEGIN_NAMESPACE
namespace Ui { class ContractVisitClass; };
QT_END_NAMESPACE

class ContractVisit : public QWidget
{
	Q_OBJECT

public:
    enum class Pages
    {
        VisitList       = 0,
        VisitDetails    = 1
    };

	ContractVisit(QWidget *parent = nullptr);
	~ContractVisit();

    void LoadTableData();

private:
    void LoadVisitTable();
    void LoadVisitToEdit(std::uint64_t id);
    void RemoveWorker(std::uint64_t workerId);

    bool CheckWorkerData();
    void UpdateButtonsManuallyWorkerAdd();
    void UpdateButtonSave();

    void CollectEditChanges();
    bool HasChanges();

    void LoadStatusComboBox();
    void SetStatus(std::uint16_t statusId);

	Ui::ContractVisitClass *ui;

    ContractorVisitModel* _visitModel;
    ContractorVisitProxyModel* _visitProxy;
    ContractorVisitWorkerModel* _assignedWorkerModel;
    ContractorVisitStatusModel* _statusModel;

    ContractorWorkerData* _workerDataWidget{nullptr};
    QPointer<CompanyContactData> _companyContactWidget{nullptr};

    std::unique_ptr<ContractorVisitManager> _visitMgr{nullptr};

    std::vector<ContractorVisitWorkerInfo> _assignedWorkers;
    std::unordered_set<std::uint64_t> _assignedWorkerIDs;

    std::unordered_set<std::uint64_t> _workersToAdd;
    std::unordered_set<std::uint64_t> _workersToRemove;

    std::uint32_t _selectedCompanyId{0};
    std::string _selectCompanyName {};

    ContractorVisitInformation _editVisitInfo {};
    ContractorVisitInformation _originalVisitInfo {};
    bool _isEditMode = false;

    std::uint64_t _selectedWorkerId = 0;

private slots:
    void onPushAddNew();
    void onReceiveWorkerData(std::uint32_t workerID, std::string firstName, std::string lastName, std::string phone, std::string companyName);
    void onPushSelectWorker();
    void onReceiveCompanyData(std::uint32_t id, const QString& companyName);
    void onPushSelectCompany();
    void onPushNewWorker();
    void onArrivalAtTimeChanged(const QDateTime& datetime);
    void onDepartureAtTimeChanged(const QDateTime& datetime);
    void onPushSaveVisit();
    void onPushSetArrivalTime();
    void onPushSetDepartureTime();
    void onPushRemoveWorker();
    void onSelectedWorkerRowChanged(const QModelIndex& current, const QModelIndex&);
    void onPushBack();
    
};

