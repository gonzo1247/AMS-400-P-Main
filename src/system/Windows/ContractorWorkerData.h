#pragma once

#include <QWidget>

#include <memory>
#include <optional>

#include "DatabaseDefines.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ContractorWorkerDataClass; };
QT_END_NAMESPACE

class ContractorWorkerDataManager;
class CompanyContactData;
class ContractorWorkerTableModel;
class ContractorWorkerProxyModel;

class ContractorWorkerData : public QWidget
{
	Q_OBJECT

public:
    enum class Mode : std::uint8_t
    {
        Search,
        Edit
    };

	ContractorWorkerData(QWidget *parent = nullptr, bool onlySelect = false);
	~ContractorWorkerData() override;

private:
    void SetCompanyData(std::uint32_t id, const QString& name);
    bool HasValidRequiredFields() const;
    void UpdateSaveButtonState();
    bool ConfirmMissingContactInfo();
    ContractorWorkerDataDB CollectWorkerData();
    void LoadTable();
    void LoadWorkerToEdit(std::uint32_t id);
    void LoadWorkerForSignal(std::uint32_t id);

    bool HasChanges();
    void SnapshotOriginal(const ContractorWorkerDataDB& data);
    bool ConfirmDiscardChanges();
    bool TryLeaveEditMode();
    void SetEditMode(bool editMode, std::uint32_t workerId = 0);
    void ResetEditForm();

    void SetupWorkerTableHeader();

	Ui::ContractorWorkerDataClass *ui;
    CompanyContactData* _companyContactData{nullptr};
    ContractorWorkerTableModel* _contractorModel = nullptr;
    ContractorWorkerProxyModel* _contractorProxy = nullptr;

    std::unique_ptr<ContractorWorkerDataManager> _workerDataManager;

    std::uint32_t _companyID = 0;
    std::optional<ContractorWorkerDataDB> _originalData;
    bool _editMode = false;
    std::uint32_t _editWorkerID = 0;

    bool _onlySelect = false;

private slots:
    void onPushOpenContactData();
    void onPushSave();
    void onPushNewContractor();
    void onPushBackSearch();
    void onPushDeleteWorker();

signals:
    void SendWorkerData(std::uint32_t workerID, std::string firstName, std::string lastName, std::string phone, std::string companyName);

};

