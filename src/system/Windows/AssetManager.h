#pragma once

#include <QWidget>

#include "ChangeTracker.h"
#include "LookupTableModel.h"
#include "DatabaseDefines.h"
#include "ui_AssetManager.h"

class CostUnitDataHandler;
class CompanyLocationHandler;
class ComboBoxDataLoader;
class MachineListTableModel;
class MachineListManager;
class RoomTableModel;
class AssetDataManager;

QT_BEGIN_NAMESPACE
namespace Ui { class AssetManagerClass; };
QT_END_NAMESPACE

class AssetManager : public QWidget
{
	Q_OBJECT

public:
    enum class AssetPages : std::uint8_t
    {
        ASSET_PAGE_MAIN = 0,
        ASSET_PAGE_LIST = 1,
        ASSET_PAGE_LINE = 2,  // edit page for Line, Type and Manufacturer
    };

    struct SqlUpdate
    {
        std::string sql;
        std::vector<QVariant> params;
    };

	AssetManager(QWidget *parent = nullptr);
	~AssetManager();


private:
    LookupTableModel *CreateLookupModel(std::function<std::vector<LookupTableModel::Row>(bool)> loadFn,
                                        std::function<std::uint32_t(const std::string &)> insertFn,
                                        std::function<bool(std::uint32_t, const std::string &)> updateFn,
                                        std::function<bool(std::uint32_t)> softDeleteFn,
                                        std::function<bool(std::uint32_t)> restoreFn = {},
                                        std::function<bool(std::uint32_t)> canDeleteFn = {},
                                        std::function<QString(std::uint32_t)> deleteReasonFn = {});

    void StartMachineLineTable();
    void StartMachineTypeTable();
    void StartMachineManufacturerTable();
    void StartRoomTable();

    void SetupRoomTab();

    void AddNewMachineData(LookupTableModel* model, QTableView* view, QLineEdit* searchEdit);   // line, type, manufacturer
    void DeleteMachineData(LookupTableModel* model, QTableView* view);  // line, type, manufacturer
    void RestoreMachineData(LookupTableModel* model, QTableView* view); // line, type, manufacturer

    void DeleteRoomData(RoomTableModel* model, const QTableView* view);
    void RestoreRoomData(RoomTableModel* model, QTableView* view);

    void ResetLookupView(QTableView* view, QLineEdit* searchEdit);
    void SetupLookupTab(QTableView* view, QLineEdit* searchEdit, QPushButton* resetButton,
                        QCheckBox* deletedOnlyCheckbox, QPushButton* deleteButton, QPushButton* restoreButton, LookupTableModel* model);

    void SetupLookupTableView(QTableView* view, int idColumn = 0, int nameColumn = 1, int idWidth = 60);

    std::uint32_t GetSelectedLocationId(const QComboBox* cb) const;
    bool EnsureLocationSelected(const QComboBox* cb);

    // Machine List Tab
    void SetupMachineListTab();
    void OpenMachineListDetail(std::uint32_t id);
    void SetTypeComboboxById(QComboBox* cb, int id);

    void SetupMachineChangeTracking();

    void UpdateMachineUiState();
    bool ValidateMachineDirtyFields(QString& errorText);
    SqlUpdate BuildMachineUpdateSql(int machineId, const ChangeTracker<MachineField>& tracker);
    bool ConfirmDiscardMachineChanges();

    static const char* MachineFieldToColumn(MachineField field)
    {
        switch (field)
        {
            case MachineField::Name:
                return "MachineName";
            case MachineField::Number:
                return "MachineNumber";
            case MachineField::ManufacturerNumber:
                return "ManufacturerMachineNumber";
            case MachineField::CostUnitId:
                return "CostUnitID";
            case MachineField::MachineTypeId:
                return "MachineTypeID";
            case MachineField::MachineLineId:
                return "LineID";
            case MachineField::ManufacturerId:
                return "ManufacturerID";
            case MachineField::RoomId:
                return "RoomNumber";
            case MachineField::Location:
                return "location";
            case MachineField::Info:
                return "MoreInformation";
            default:
                return "";
        }
    }


	Ui::AssetManagerClass *ui;

    ChangeTracker<MachineField> _machineTracker;

    LookupTableModel* _machineLineModel{nullptr};
    LookupTableModel* _machineTypeModel{nullptr};
    LookupTableModel* _machineManufacturerModel{nullptr};
    RoomTableModel* _roomModel{nullptr};
    MachineListTableModel* _machineListModel{nullptr};

    std::unique_ptr<AssetDataManager> _assetMgr{nullptr};
    std::unique_ptr<MachineListManager> _machineListMgr{nullptr};

    std::uint32_t _currentMachineId = 0;

    MachineInformation _currentMachineDetails;

private slots:
    void onPushAddMachineLine();
    void onPushDeleteMachineLine();
    void onPushRestoreMachineLine();

    void onPushAddMachineType();
    void onPushDeleteMachineType();
    void onPushRestoreMachineType();

    void onPushAddMachineManufacturer();
    void onPushDeleteMachineManufacturer();
    void onPushRestoreMachineManufacturer();

    void onPushAddRoom();
    void onPushDeleteRoom();
    void onPushRestoreRoom();

    void onPushBackToMainPage();

    void onPushSaveMachine();
};


