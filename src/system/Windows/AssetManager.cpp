#include "pch.h"
#include "AssetDataManager.h"
#include "AssetManager.h"
#include "ComboBoxDataLoader.h"
#include "ConnectionGuard.h"
#include "CostUnitDataHandler.h"
#include "RoomTableModel.h"
#include "SettingsManager.h"
#include "TableSearchHelper.h"
#include "MachineListTableModel.h"
#include "MachineListManager.h"
#include "UiChangeBinding.h"

AssetManager::AssetManager(QWidget *parent) : QWidget(parent), ui(new Ui::AssetManagerClass())
{
	ui->setupUi(this);

    _assetMgr = std::make_unique<AssetDataManager>();
    ComboBoxDataLoader::ReloadAndFillCompanyLocationCombo(ui->cb_lineLocation);
    ComboBoxDataLoader::ReloadAndFillCompanyLocationCombo(ui->cb_roomLocation);
    StartMachineLineTable();
    StartMachineTypeTable();
    StartMachineManufacturerTable();
    StartRoomTable();

    // machine_line tab
    {
        connect(ui->pb_newLine, &QPushButton::clicked, this, &AssetManager::onPushAddMachineLine);
        connect(ui->pb_deleteLine, &QPushButton::clicked, this, &AssetManager::onPushDeleteMachineLine);
        TableSearchHelper::Setup(ui->tv_line, ui->le_lineSearch, _machineLineModel);
        connect(ui->pb_restoreLine, &QPushButton::clicked, this, &AssetManager::onPushRestoreMachineLine);
        SetupLookupTab(ui->tv_line, ui->le_lineSearch, ui->pb_lineReset, ui->chb_deletedLines, ui->pb_deleteLine, ui->pb_restoreLine, _machineLineModel);
        connect(ui->cb_lineLocation, &QComboBox::currentIndexChanged, this,
                [this]()
                {
                    if (_machineLineModel)
                        _machineLineModel->reload();
                });        
    }

    // machine_type tab
    {
        connect(ui->pb_newType, &QPushButton::clicked, this, &AssetManager::onPushAddMachineType);
        connect(ui->pb_deleteType, &QPushButton::clicked, this, &AssetManager::onPushDeleteMachineType);
        TableSearchHelper::Setup(ui->tv_type,  ui->le_typeSearch, _machineTypeModel);
        connect(ui->pb_restoreType, &QPushButton::clicked, this, &AssetManager::onPushRestoreMachineType);
        SetupLookupTab(ui->tv_type, ui->le_typeSearch, ui->pb_typeReset, ui->chb_deletedType, ui->pb_deleteType, ui->pb_restoreType, _machineTypeModel);
    }

    // machine_manufacturer tab
    {
        connect(ui->pb_newManufacturer, &QPushButton::clicked, this, &AssetManager::onPushAddMachineManufacturer);
        connect(ui->pb_deleteManufacturer, &QPushButton::clicked, this, &AssetManager::onPushDeleteMachineManufacturer);
        TableSearchHelper::Setup(ui->tv_manufacturer, ui->le_manufacturerSearch, _machineManufacturerModel);
        connect(ui->pb_restoreManufacturer, &QPushButton::clicked, this, &AssetManager::onPushRestoreMachineManufacturer);
        SetupLookupTab(ui->tv_manufacturer, ui->le_manufacturerSearch, ui->pb_manufacturerReset, ui->chb_deletedManufacturer,
                       ui->pb_deleteManufacturer, ui->pb_restoreManufacturer, _machineManufacturerModel);
    }

    SetupRoomTab();

    // Table header setups
    {
        SetupLookupTableView(ui->tv_line);
        SetupLookupTableView(ui->tv_type);
        SetupLookupTableView(ui->tv_manufacturer);
    }

    {
        SetupMachineListTab();
        TableSearchHelper::Setup(ui->tv_list, ui->le_listSearch, _machineListModel);
        ui->pb_backMainPage->setEnabled(false);
        connect(ui->tv_list, &QTableView::doubleClicked, this,
            [this](const QModelIndex &index)
            {
                if (!index.isValid())
                    return;

                const auto id = index.data(Qt::UserRole).toLongLong();
                if (id <= 0)
                    return;

                // Use id (open editor, details, etc.)
                OpenMachineListDetail(static_cast<std::uint32_t>(id));
            });
    }

    connect(ui->pb_backMainPage, &QPushButton::clicked, this, &AssetManager::onPushBackToMainPage);
    connect(ui->pb_save, &QPushButton::clicked, this, &AssetManager::onPushSaveMachine);
    SetupMachineChangeTracking();
    connect(ui->pb_newMachine, &QPushButton::clicked, this, &AssetManager::onPushNewMachine);
}

AssetManager::~AssetManager()
{
	delete ui;
}

LookupTableModel* AssetManager::CreateLookupModel(std::function<std::vector<LookupTableModel::Row>(bool)> loadFn,
                                                  std::function<std::uint32_t(const std::string &)> insertFn,
                                                  std::function<bool(std::uint32_t, const std::string &)> updateFn,
                                                  std::function<bool(std::uint32_t)> softDeleteFn,
                                                  std::function<bool(std::uint32_t)> restoreFn /*= {}*/,
                                                  std::function<bool(std::uint32_t)> canDeleteFn /*= {}*/,
                                                  std::function<QString(std::uint32_t)> deleteReasonFn /*= {}*/)
{
    LookupTableModel::Provider p;
    p.load = std::move(loadFn);
    p.insert = std::move(insertFn);
    p.updateName = std::move(updateFn);
    p.softDelete = std::move(softDeleteFn);
    p.restore = std::move(restoreFn);
    p.canDelete = std::move(canDeleteFn);
    p.deleteBlockReason = std::move(deleteReasonFn);

    return new LookupTableModel(std::move(p), this);
}

void AssetManager::StartMachineLineTable()
{
    LookupTableModel::Provider p;

    // Scope: current location
    p.scopeId = [this]() { return GetSelectedLocationId(ui->cb_lineLocation); };

    // Scoped load
    p.loadScoped = [this](std::uint32_t locationId, bool deletedOnly)
    { return _assetMgr->LoadMachineLines(locationId, deletedOnly); };

    // Scoped insert
    p.insertScoped = [this](std::uint32_t locationId, const std::string &name)
    { return _assetMgr->AddMachineLine(locationId, name); };

    // unscoped, id is enough
    p.updateName = [this](std::uint32_t id, const std::string &name)
    { return _assetMgr->UpdateMachineLineName(id, name); };

    p.softDelete = [this](std::uint32_t id) { return _assetMgr->SoftDeleteMachineLine(id); };

    p.restore = [this](std::uint32_t id) { return _assetMgr->RestoreMachineLine(id); };

    // Optional (not scoped - id suffices)
    p.canDelete = [this](std::uint32_t id) { return _assetMgr->CanDeleteMachineLine(id); };

    p.deleteBlockReason = [this](std::uint32_t id) { return _assetMgr->GetMachineLineDeleteBlockReason(id); };

    _machineLineModel = new LookupTableModel(std::move(p), this);

    ui->tv_line->setModel(_machineLineModel);
    _machineLineModel->reload();
}

void AssetManager::StartMachineTypeTable()
{
    _machineTypeModel =
        CreateLookupModel([this](bool inc) { return _assetMgr->LoadMachineTypes(inc); }, [this](const std::string &n)
                          { return _assetMgr->AddMachineType(n); }, [this](std::uint32_t id, const std::string &n)
                          { return _assetMgr->UpdateMachineTypeName(id, n); },
                          [this](std::uint32_t id) { return _assetMgr->SoftDeleteMachineType(id); },
                          [this](std::uint32_t id) { return _assetMgr->RestoreMachineType(id); },
                          [this](std::uint32_t id) { return _assetMgr->CanDeleteMachineType(id); },
                          [this](std::uint32_t id) { return _assetMgr->GetMachineTypeDeleteBlockReason(id); });

    ui->tv_type->setModel(_machineTypeModel);
    _machineTypeModel->reload();
}

void AssetManager::StartMachineManufacturerTable()
{
    _machineManufacturerModel =
        CreateLookupModel([this](bool inc) { return _assetMgr->LoadMachineManufacturers(inc); }, [this](const std::string &n)
                          { return _assetMgr->AddMachineManufacturer(n); }, [this](std::uint32_t id, const std::string &n)
                          { return _assetMgr->UpdateMachineManufacturerName(id, n); },
                          [this](std::uint32_t id) { return _assetMgr->SoftDeleteMachineManufacturer(id); },
                          [this](std::uint32_t id) { return _assetMgr->RestoreMachineManufacturer(id); },
                          [this](std::uint32_t id) { return _assetMgr->CanDeleteMachineManufacturer(id); },
                          [this](std::uint32_t id) { return _assetMgr->GetMachineManufacturerDeleteBlockReason(id); });

    ui->tv_manufacturer->setModel(_machineManufacturerModel);
    _machineManufacturerModel->reload();
}

void AssetManager::StartRoomTable()
{
    RoomTableModel::Provider p;

    p.scopeId = [this]() { return GetSelectedLocationId(ui->cb_roomLocation); };

    p.loadScoped = [this](std::uint32_t locationId, bool deletedOnly){ return _assetMgr->LoadRooms(locationId, deletedOnly); };
 
    p.insertScoped = [this](std::uint32_t locationId, const std::string &code, const std::string &name)
    { return _assetMgr->AddRoom(locationId, code, name); };

    // id is unique, scope not required for these
    p.update = [this](std::uint32_t id, const std::string &code, const std::string &name)
    { return _assetMgr->UpdateRoom(id, code, name); };

    p.softDelete = [this](std::uint32_t id) { return _assetMgr->SoftDeleteRoom(id); };

    p.restore = [this](std::uint32_t id) { return _assetMgr->RestoreRoom(id); };

    p.canDelete = [this](std::uint32_t id) { return _assetMgr->CanDeleteRoom(id); };

    p.deleteBlockReason = [this](std::uint32_t id) { return _assetMgr->GetRoomDeleteBlockReason(id); };

    _roomModel = new RoomTableModel(std::move(p), this);

    ui->tv_room->setModel(_roomModel);
    _roomModel->reload();
}

void AssetManager::SetupRoomTab()
{
    connect(ui->pb_newRoom, &QPushButton::clicked, this, &AssetManager::onPushAddRoom);

    connect(ui->pb_deleteRoom, &QPushButton::clicked, this, &AssetManager::onPushDeleteRoom);

    connect(ui->pb_restoreRoom, &QPushButton::clicked, this, &AssetManager::onPushRestoreRoom);

    connect(ui->pb_resetRoom, &QPushButton::clicked, this, [this]() { ResetLookupView(ui->tv_room, ui->le_roomSearch); });

    connect(ui->chb_deletedRooms, &QCheckBox::toggled, this,
            [this](bool checked)
            {
                if (_roomModel)
                    _roomModel->setIncludeDeleted(checked);

                ui->pb_restoreRoom->setEnabled(checked);
                ui->pb_deleteRoom->setEnabled(!checked);

                ResetLookupView(ui->tv_room, ui->le_roomSearch);
            });

    connect(ui->cb_roomLocation, &QComboBox::currentIndexChanged, this,
            [this]()
            {
                if (_roomModel)
                    _roomModel->reload();
            });

    // Enable restore only when a deleted row is selected and deleted-only is active
    if (ui->tv_room->selectionModel())
    {
        connect(ui->tv_room->selectionModel(), &QItemSelectionModel::currentChanged, this,
                [this](const QModelIndex &current, const QModelIndex &)
                {
                    if (!_roomModel)
                        return;

                    const bool allowRestore = ui->chb_deletedRooms->isChecked() && current.isValid();
                    if (!allowRestore)
                    {
                        ui->pb_restoreRoom->setEnabled(false);
                        return;
                    }

                    const QModelIndex srcIdx = TableSearchHelper::MapToSource(ui->tv_room, current);
                    const auto *row = srcIdx.isValid() ? _roomModel->rowAt(srcIdx.row()) : nullptr;

                    ui->pb_restoreRoom->setEnabled(row && row->isDeleted);
                });
    }

    ui->pb_restoreRoom->setEnabled(ui->chb_deletedRooms->isChecked());
    ui->pb_deleteRoom->setEnabled(!ui->chb_deletedRooms->isChecked());

    // Search: if you already have a helper for QSortFilterProxyModel, use it here
    TableSearchHelper::Setup(ui->tv_room, ui->le_roomSearch, _roomModel);

    // Column sizing: ID small, code small-ish, name stretch
    auto *header = ui->tv_room->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionResizeMode(static_cast<int>(RoomTableModel::Column::Id), QHeaderView::Fixed);
    header->setSectionResizeMode(static_cast<int>(RoomTableModel::Column::Code), QHeaderView::ResizeToContents);
    header->setSectionResizeMode(static_cast<int>(RoomTableModel::Column::Name), QHeaderView::Stretch);

    ui->tv_room->setColumnWidth(static_cast<int>(RoomTableModel::Column::Id), 60);
}

void AssetManager::AddNewMachineData(LookupTableModel *model, QTableView *view, QLineEdit *searchEdit)
{
    if (!model || !view)
        return;

    const std::uint32_t newId = model->addNew(QStringLiteral("New"));
    if (newId == 0)
        return;

    // Keep the new row visible even if a filter is active
    if (searchEdit)
        searchEdit->setText(QString::number(newId));

    int srcRow = -1;
    for (int r = 0; r < model->rowCount(); ++r)
    {
        const auto *rowData = model->rowAt(r);
        if (rowData && rowData->id == newId)
        {
            srcRow = r;
            break;
        }
    }

    if (srcRow < 0)
        return;

    const QModelIndex srcIdx = model->index(srcRow, static_cast<int>(LookupTableModel::Column::Name));

    auto *proxy = qobject_cast<QSortFilterProxyModel *>(view->model());
    const QModelIndex viewIdx = proxy ? proxy->mapFromSource(srcIdx) : srcIdx;

    if (!viewIdx.isValid())
        return;

    view->setCurrentIndex(viewIdx);
    view->scrollTo(viewIdx);
    view->edit(viewIdx);
}

void AssetManager::DeleteMachineData(LookupTableModel *model, QTableView *view)
{
    if (!model || !view)
        return;

    const QModelIndex viewIdx = view->currentIndex();
    if (!viewIdx.isValid())
        return;

    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, viewIdx);
    if (!srcIdx.isValid())
        return;

    if (QMessageBox::question(this, tr("Delete"), tr("Do you really want to delete this entry?")) != QMessageBox::Yes)
        return;

    const int srcRow = srcIdx.row();
    if (!model->softDeleteRow(srcRow))
        return;

    // Select next reasonable row in the view (proxy-safe)
    QAbstractItemModel *viewModel = view->model();
    if (!viewModel)
        return;

    const int viewRowCount = viewModel->rowCount();
    if (viewRowCount <= 0)
        return;

    int nextViewRow = viewIdx.row();
    if (nextViewRow >= viewRowCount)
        nextViewRow = viewRowCount - 1;

    const QModelIndex nextIdx = viewModel->index(nextViewRow, viewIdx.column());
    if (!nextIdx.isValid())
        return;

    view->setCurrentIndex(nextIdx);
    view->scrollTo(nextIdx);
}

void AssetManager::RestoreMachineData(LookupTableModel *model, QTableView *view)
{
    if (!model || !view)
        return;

    const QModelIndex viewIdx = view->currentIndex();
    if (!viewIdx.isValid())
        return;

    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, viewIdx);
    if (!srcIdx.isValid())
        return;

    if (QMessageBox::question(this, tr("Restore"), tr("Do you really want to restore this entry?")) != QMessageBox::Yes)
        return;

    const int srcRow = srcIdx.row();
    if (!model->restoreRow(srcRow))
        return;

    // Keep a valid selection after refresh
    QAbstractItemModel *viewModel = view->model();
    if (!viewModel)
        return;

    const int rowCount = viewModel->rowCount();
    if (rowCount <= 0)
        return;

    int nextRow = viewIdx.row();
    if (nextRow >= rowCount)
        nextRow = rowCount - 1;

    const QModelIndex nextIdx = viewModel->index(nextRow, viewIdx.column());
    if (!nextIdx.isValid())
        return;

    view->setCurrentIndex(nextIdx);
    view->scrollTo(nextIdx);
}

void AssetManager::DeleteRoomData(RoomTableModel *model, const QTableView *view)
{
    if (!model || !view)
        return;

    const QModelIndex idx = view->currentIndex();
    if (!idx.isValid())
        return;

    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, idx);
    if (!srcIdx.isValid())
        return;

    if (QMessageBox::question(this, tr("Delete"), tr("Do you really want to delete this entry?")) != QMessageBox::Yes)
        return;

    model->softDeleteRow(srcIdx.row());
}

void AssetManager::RestoreRoomData(RoomTableModel *model, QTableView *view)
{
    if (!model || !view)
        return;

    const QModelIndex idx = view->currentIndex();
    if (!idx.isValid())
        return;

    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, idx);
    if (!srcIdx.isValid())
        return;

    if (QMessageBox::question(this, tr("Restore"), tr("Do you really want to restore this entry?")) != QMessageBox::Yes)
        return;

    model->restoreRow(srcIdx.row());

    const QModelIndex nextIdx = view->currentIndex();
    if (nextIdx.isValid())
    {
        view->scrollTo(nextIdx);
        view->setCurrentIndex(nextIdx);
    }
}

void AssetManager::ResetLookupView(QTableView *view, QLineEdit *searchEdit)
{
    if (!view)
        return;

    // Clear search/filter
    if (searchEdit)
        searchEdit->clear();

    QAbstractItemModel *model = view->model();
    if (!model)
        return;

    if (model->rowCount() <= 0)
        return;

    const QModelIndex firstIdx = model->index(0, 0);
    if (!firstIdx.isValid())
        return;

    view->setCurrentIndex(firstIdx);
    view->scrollTo(firstIdx);
    view->setFocus();
}

void AssetManager::SetupLookupTab(QTableView *view, QLineEdit *searchEdit, QPushButton *resetButton,
                                  QCheckBox *deletedOnlyCheckbox, QPushButton *deleteButton, QPushButton *restoreButton,
                                  LookupTableModel *model)
{
    if (!view || !deletedOnlyCheckbox || !deleteButton || !restoreButton || !model)
        return;

    // Deleted-only toggle
    connect(deletedOnlyCheckbox, &QCheckBox::toggled, this,
            [this, model, view, searchEdit, deletedOnlyCheckbox, deleteButton, restoreButton](bool checked)
            {
                model->setIncludeDeleted(checked);

                deleteButton->setEnabled(!checked);

                const QModelIndex current = view->currentIndex();
                const bool allowRestore = checked && current.isValid();

                if (!allowRestore)
                {
                    restoreButton->setEnabled(false);
                }
                else
                {
                    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, current);
                    const auto *row = srcIdx.isValid() ? model->rowAt(srcIdx.row()) : nullptr;
                    restoreButton->setEnabled(row && row->isDeleted);
                }

                ResetLookupView(view, searchEdit);
            });

    // Restore enabled only when a deleted row is selected and deleted-only is active
    if (view->selectionModel())
    {
        connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this,
                [model, view, deletedOnlyCheckbox, restoreButton](const QModelIndex &current, const QModelIndex &)
                {
                    const bool allowRestore = deletedOnlyCheckbox->isChecked() && current.isValid();
                    if (!allowRestore)
                    {
                        restoreButton->setEnabled(false);
                        return;
                    }

                    const QModelIndex srcIdx = TableSearchHelper::MapToSource(view, current);
                    const auto *row = srcIdx.isValid() ? model->rowAt(srcIdx.row()) : nullptr;

                    restoreButton->setEnabled(row && row->isDeleted);
                });
    }

    // Reset button
    if (resetButton)
    {
        connect(resetButton, &QPushButton::clicked, this,
                [this, view, searchEdit]() { ResetLookupView(view, searchEdit); });
    }

    // Initial button state
    deleteButton->setEnabled(!deletedOnlyCheckbox->isChecked());
    restoreButton->setEnabled(deletedOnlyCheckbox->isChecked() && view->currentIndex().isValid());
}

void AssetManager::SetupLookupTableView(QTableView *view, int idColumn /*= 0*/, int nameColumn /*= 1*/, int idWidth /*= 60*/)
{
    if (!view)
        return;

    auto *header = view->horizontalHeader();

    view->setWordWrap(false);
    view->setTextElideMode(Qt::ElideRight);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setAlternatingRowColors(true);

    header->setHighlightSections(false);
    header->setStretchLastSection(false);

    // Default: user can still resize
    header->setSectionResizeMode(QHeaderView::Interactive);

    // ID column: small & stable
    header->setSectionResizeMode(idColumn, QHeaderView::Fixed);
    view->setColumnWidth(idColumn, idWidth);

    // Name column: takes remaining space
    header->setSectionResizeMode(nameColumn, QHeaderView::Stretch);

    // Optional: auto-size other columns if any exist
    const int colCount = view->model() ? view->model()->columnCount() : 0;
    for (int c = 0; c < colCount; ++c)
    {
        if (c == idColumn || c == nameColumn)
            continue;

        header->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }

    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

std::uint32_t AssetManager::GetSelectedLocationId(const QComboBox* cb) const
{
    if (!cb)
        return 0;

    const QVariant v = cb->currentData();
    if (!v.isValid())
        return 0;

    return v.toUInt();
}

bool AssetManager::EnsureLocationSelected(const QComboBox* cb)
{
    const std::uint32_t locationId = GetSelectedLocationId(cb);
    if (locationId != 0)
        return true;

    QMessageBox::warning(this, tr("Location missing"), tr("Please select a valid location."));
    return false;
}

void AssetManager::SetupMachineListTab()
{
    if (!_machineListMgr)
        _machineListMgr = std::make_unique<MachineListManager>();

    if (!_machineListModel)
        _machineListModel = new MachineListTableModel(this);

    ui->tv_list->setModel(_machineListModel);

    auto task = [this]()    
    {
        _machineListMgr->LoadMachineListFromDatabase();
        auto data = _machineListMgr->GetMachineData();

        QMetaObject::invokeMethod(
            this,
            [this, data = std::move(data)]() mutable
            {
                _machineListModel->setData(std::move(data));

                ui->tv_list->setSelectionBehavior(QAbstractItemView::SelectRows);
                ui->tv_list->setSelectionMode(QAbstractItemView::SingleSelection);

            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(std::move(task), this);
}

void AssetManager::OpenMachineListDetail(std::uint32_t id)
{
    _currentMachineId = id;
    auto task = [this, id]()
    {
        auto ptr = _machineListMgr->GetById(id);

        if (!ptr)
            return;

        MachineInformation detailsCopy = *ptr;
        
        QMetaObject::invokeMethod(
            this,
            [this, details = std::move(detailsCopy)]() mutable
            {
                ChangeTracker<MachineField>::SuspendGuard guard(_machineTracker);
                _currentMachineDetails = details;

                ui->pb_backMainPage->setEnabled(true);

                // Machine Main Data
                ui->le_machineName->setText(QString::fromStdString(details.MachineName));
                ui->le_machineNumber->setText(QString::fromStdString(details.MachineNumber));
                ui->le_manufacturerNumber->setText(QString::fromStdString(details.ManufacturerMachineNumber));

                auto location = details.locationID != 0 ? static_cast<CompanyLocations>(details.locationID) : CompanyLocations::CL_BBG;

                FillMachineListComboBox(location);

                // Machine Sub Data
                SetTypeComboboxById(ui->cb_room, details.RoomID);
                SetTypeComboboxById(ui->cb_machineLine, details.LineID);
                SetTypeComboboxById(ui->cb_manufacturer, details.ManufacturerID);
                SetTypeComboboxById(ui->cb_machineType, details.MachineTypeID);
                SetTypeComboboxById(ui->cb_costUnit, details.CostUnitID);
                SetTypeComboboxById(ui->cb_location, static_cast<int>(location));

                // Other Data
                ui->te_information->setText(QString::fromStdString(details.MoreInformation));

                ui->sw_assetManager->setCurrentIndex(static_cast<int>(AssetPages::ASSET_PAGE_LIST));

                ResetDirtyStatus();


            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(task, this);
}

void AssetManager::SetTypeComboboxById(QComboBox *cb, int id)
{
    if (!cb)
        return;

    const int index = cb->findData(id);
    if (index >= 0)
        cb->setCurrentIndex(index);
}

void AssetManager::SetupMachineChangeTracking()
{
    auto onChanged = [this]() { UpdateMachineUiState(); };

    BindLineEdit(ui->le_machineName, _machineTracker, MachineField::Name, onChanged);
    BindLineEdit(ui->le_machineNumber, _machineTracker, MachineField::Number, onChanged);
    BindLineEdit(ui->le_manufacturerNumber, _machineTracker, MachineField::ManufacturerNumber, onChanged);

    BindComboBox(ui->cb_costUnit, _machineTracker, MachineField::CostUnitId, onChanged);
    BindComboBox(ui->cb_machineType, _machineTracker, MachineField::MachineTypeId, onChanged);
    BindComboBox(ui->cb_machineLine, _machineTracker, MachineField::MachineLineId, onChanged);
    BindComboBox(ui->cb_manufacturer, _machineTracker, MachineField::ManufacturerId, onChanged);
    BindComboBox(ui->cb_room, _machineTracker, MachineField::RoomId, onChanged);
    BindComboBox(ui->cb_location, _machineTracker, MachineField::Location, onChanged);

    BindTextEdit(ui->te_information, _machineTracker, MachineField::Info, onChanged);
}

void AssetManager::UpdateMachineUiState()
{
    const bool dirty = _machineTracker.IsDirty();

    ui->pb_save->setEnabled(dirty);

    if (!dirty)
        ui->lb_machineDirtyHint->clear();
    else
        ui->lb_machineDirtyHint->setText(tr("Unsaved changes"));
}

bool AssetManager::ValidateMachineDirtyFields(QString &errorText)
{
    const auto dirty = _machineTracker.GetDirtyFields();
    if (dirty.isEmpty())
        return true;

    auto fail = [&](const QString &msg)
    {
        errorText = msg;
        return false;
    };

    if (dirty.contains(MachineField::Name))
    {
        const QString name = ui->le_machineName->text().trimmed();
        if (name.isEmpty())
            return fail(tr("Machine name must not be empty."));
        if (name.size() > 200)
            return fail(tr("Machine name is too long (max 200)."));
    }

    if (dirty.contains(MachineField::Number))
    {
        const QString num = ui->le_machineNumber->text().trimmed();
        if (num.size() > 50)
            return fail(tr("Machine number is too long (max 50)."));
    }

    if (dirty.contains(MachineField::ManufacturerNumber))
    {
        const QString num = ui->le_manufacturerNumber->text().trimmed();
        if (num.size() > 50)
            return fail(tr("Manufacturer machine number is too long (max 50)."));
    }

    if (dirty.contains(MachineField::Info))
    {
        const QString info = ui->te_information->toPlainText();
        if (info.size() > 255)
            return fail(tr("More information is too long (max 255)."));
    }

    return true;
}

bool AssetManager::ConfirmDiscardMachineChanges()
{
    if (!_machineTracker.IsDirty())
        return true;

    const auto ret =
        QMessageBox::warning(this, tr("Unsaved changes"), tr("There are unsaved changes. Do you want to discard them?"),
                             QMessageBox::Yes | QMessageBox::No);

    return ret == QMessageBox::Yes;
}

void AssetManager::ClearFieldsForMachine()
{
    ui->le_machineName->clear();
    ui->le_machineNumber->clear();
    ui->le_manufacturerNumber->clear();
    ui->cb_costUnit->setCurrentIndex(-1);
    ui->cb_machineType->setCurrentIndex(-1);
    ui->cb_machineLine->setCurrentIndex(-1);
    ui->cb_manufacturer->setCurrentIndex(-1);
    ui->cb_room->setCurrentIndex(-1);
    ui->cb_location->setCurrentIndex(-1);
    ui->te_information->clear();

    ui->lb_machineDirtyHint->clear();
}

void AssetManager::ResetDirtyStatus()
{
    ChangeTracker<MachineField>::SuspendGuard guard(_machineTracker);

    _machineTracker.SetValueForced(MachineField::Name, ui->le_machineName->text());
    _machineTracker.SetValueForced(MachineField::Number, ui->le_machineNumber->text());
    _machineTracker.SetValueForced(MachineField::ManufacturerNumber, ui->le_manufacturerNumber->text());
    _machineTracker.SetValueForced(MachineField::CostUnitId, ui->cb_costUnit->currentData());
    _machineTracker.SetValueForced(MachineField::MachineTypeId, ui->cb_machineType->currentData());
    _machineTracker.SetValueForced(MachineField::MachineLineId, ui->cb_machineLine->currentData());
    _machineTracker.SetValueForced(MachineField::ManufacturerId, ui->cb_manufacturer->currentData());
    _machineTracker.SetValueForced(MachineField::Info, ui->te_information->toPlainText());
    _machineTracker.SetValueForced(MachineField::RoomId, ui->cb_room->currentData());
    _machineTracker.SetValueForced(MachineField::Location, ui->cb_location->currentData());

    _machineTracker.BeginSnapshot();
}

QString AssetManager::ToUiMessage(AssetDataManager::MachineValidationResult r, QObject *ctx)
{
    switch (r.error)
    {
        case AssetDataManager::MachineValidationError::None:
            return {};

        case AssetDataManager::MachineValidationError::NameEmpty:
            return ctx->tr("Machine name must not be empty.");

        case AssetDataManager::MachineValidationError::NameTooLong:
            return ctx->tr("Machine name is too long (max %1).").arg(static_cast<int>(r.limit));

        case AssetDataManager::MachineValidationError::NumberTooLong:
            return ctx->tr("Machine number is too long (max %1).").arg(static_cast<int>(r.limit));

        case AssetDataManager::MachineValidationError::ManufacturerNumberTooLong:
            return ctx->tr("Manufacturer machine number is too long (max %1).").arg(static_cast<int>(r.limit));

        case AssetDataManager::MachineValidationError::InfoTooLong:
            return ctx->tr("More information is too long (max %1).").arg(static_cast<int>(r.limit));
    }

    // Fallback
    return ctx->tr("Invalid machine data.");
}

MachineInformation AssetManager::CollectMachineData()
{
    MachineInformation info = _currentMachineDetails;

    info.ID = static_cast<std::int32_t>(_currentMachineId);
    info.MachineName = ui->le_machineName->text().trimmed().toStdString();
    info.MachineNumber = ui->le_machineNumber->text().trimmed().toStdString();
    info.ManufacturerMachineNumber = ui->le_manufacturerNumber->text().trimmed().toStdString();
    info.CostUnitID = ui->cb_costUnit->currentData().toUInt();
    info.MachineTypeID = ui->cb_machineType->currentData().toUInt();
    info.LineID = ui->cb_machineLine->currentData().toUInt();
    info.ManufacturerID = ui->cb_manufacturer->currentData().toUInt();
    info.RoomID = ui->cb_room->currentData().toUInt();
    info.locationID = ui->cb_location->currentData().toUInt();
    info.MoreInformation = ui->te_information->toPlainText().toStdString();

    return info;
}

void AssetManager::FillComboBoxWithCompleter(QComboBox *cb, const std::vector<AssetDataManager::ComboEntry> &items)
{
    if (!cb)
        return;

    cb->clear();

    QStringList list;
    list.reserve(static_cast<int>(items.size()));

    for (const auto &it : items)
    {
        const QString text = QString::fromStdString(it.text);

        cb->addItem(text, static_cast<int>(it.id));
        list.append(text);
    }

    if (auto *old = cb->completer())
        old->deleteLater();

    auto *completer = new QCompleter(list, cb);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    cb->setCompleter(completer);
}

void AssetManager::FillMachineListComboBox(CompanyLocations cl /*= CompanyLocations::CL_BBG*/)
{
    auto rooms = _assetMgr->GetRoomsForLocation(cl);
    FillComboBoxWithCompleter(ui->cb_room, rooms);

    auto lines = _assetMgr->GetLineForBox(cl);
    FillComboBoxWithCompleter(ui->cb_machineLine, lines);

    auto manufacturer = _assetMgr->GetManufacturerForBox();
    FillComboBoxWithCompleter(ui->cb_manufacturer, manufacturer);

    auto types = _assetMgr->GetTypeForBox();
    FillComboBoxWithCompleter(ui->cb_machineType, types);

    CostUnitDataHandler::instance().FillComboBoxWithData(ui->cb_costUnit, GetSettings().getLanguage(), GetCompanyLocationsString(cl));
    ComboBoxDataLoader::ReloadAndFillCompanyLocationCombo(ui->cb_location);
}

void AssetManager::onPushAddMachineLine()
{
    if (!EnsureLocationSelected(ui->cb_lineLocation))
        return;

    AddNewMachineData(_machineLineModel, ui->tv_line, ui->le_lineSearch);
}

void AssetManager::onPushDeleteMachineLine()
{
    if (!EnsureLocationSelected(ui->cb_lineLocation))
        return;

    DeleteMachineData(_machineLineModel, ui->tv_line);
}

void AssetManager::onPushRestoreMachineLine()
{
    if (!EnsureLocationSelected(ui->cb_lineLocation))
        return;

    RestoreMachineData(_machineLineModel, ui->tv_line);
    ResetLookupView(ui->tv_line, ui->le_lineSearch);
}

void AssetManager::onPushAddMachineType()
{
    AddNewMachineData(_machineTypeModel, ui->tv_type, ui->le_typeSearch);
}

void AssetManager::onPushDeleteMachineType()
{
    DeleteMachineData(_machineTypeModel, ui->tv_type);
}

void AssetManager::onPushRestoreMachineType()
{
    RestoreMachineData(_machineTypeModel, ui->tv_type);
    ResetLookupView(ui->tv_type, ui->le_typeSearch);
}

void AssetManager::onPushAddMachineManufacturer()
{
    AddNewMachineData(_machineManufacturerModel, ui->tv_manufacturer, ui->le_manufacturerSearch);
}

void AssetManager::onPushDeleteMachineManufacturer()
{
    DeleteMachineData(_machineManufacturerModel, ui->tv_manufacturer);
}

void AssetManager::onPushRestoreMachineManufacturer()
{
    RestoreMachineData(_machineManufacturerModel, ui->tv_manufacturer);
    ResetLookupView(ui->tv_manufacturer, ui->le_manufacturerSearch);
}

void AssetManager::onPushAddRoom()
{
    if (!EnsureLocationSelected(ui->cb_roomLocation))
        return;

    if (!_roomModel)
        return;

    const std::uint32_t locationId = ui->cb_roomLocation->currentData().toUInt();
    if (locationId == 0)
        return;

    const std::uint32_t newId = _roomModel->addNew(QStringLiteral("000"), QStringLiteral("New"));
    if (newId == 0)
        return;

    ResetLookupView(ui->tv_room, ui->le_roomSearch);
}

void AssetManager::onPushDeleteRoom()
{
    if (!EnsureLocationSelected(ui->cb_roomLocation))
        return;

    if (!_roomModel)
        return;

    DeleteRoomData(_roomModel, ui->tv_room);
}

void AssetManager::onPushRestoreRoom()
{
    if (!EnsureLocationSelected(ui->cb_roomLocation))
        return;

    if (!_roomModel)
        return;

    RestoreRoomData(_roomModel, ui->tv_room);
}

void AssetManager::onPushBackToMainPage()
{
    if (!ConfirmDiscardMachineChanges())
        return;

    ui->sw_assetManager->setCurrentIndex(static_cast<int>(AssetPages::ASSET_PAGE_MAIN));
    ui->pb_backMainPage->setEnabled(false);

    if (_isNewMachine)
    {
        _isNewMachine = false;
        ClearFieldsForMachine();
    }
}

void AssetManager::onPushSaveMachine()
{
    if (!_isNewMachine && !_machineTracker.IsDirty())
        return;

    auto info = CollectMachineData();

    const auto vr = _assetMgr->Validate(info);
    if (vr.error != AssetDataManager::MachineValidationError::None)
    {
        QMessageBox::warning(this, tr("Validation"), ToUiMessage(vr, this));
        return;
    }

    if (_isNewMachine)
    {
        Q_ASSERT(info.ID == 0);
    }
    else
    {
        Q_ASSERT(_currentMachineDetails.ID == info.ID);
    }

    if (!_isNewMachine)
    {        
        const bool ok = _assetMgr->UpdateMachineData(info, _machineTracker);        
        if (!ok)
        {
            QMessageBox::critical(this, tr("Save"), tr("Failed to save machine changes."));
            return;
        }

        QMessageBox::information(this, tr("Save"), tr("Machine changes saved successfully."));

        // Reset dirty state
        ResetDirtyStatus();

        // 1) Update manager cache
        _machineListMgr->UpdateCachedMachine(info);

        // 2) Patching TableModel (UI)
        _machineListModel->ApplyMachinePatch(info);

        _currentMachineDetails = info;

        UpdateMachineUiState();

        return;
    }

    
    auto [newId, success] = _assetMgr->AddNewMachine(info);

    if (!success || newId == 0)
    {
        QMessageBox::critical(this, tr("Add Machine"), tr("Failed to add new machine."));
        return;
    }

    QMessageBox::information(this, tr("Add Machine"), tr("New machine added successfully."));

    info.ID = static_cast<std::int32_t>(newId);

    _machineListMgr->AddNewMachineToCache(info);

    _machineListModel->UpsertMachine(info);

    ClearFieldsForMachine();
    ResetDirtyStatus();
    UpdateMachineUiState();

    if (QMessageBox::information(this, tr("Add Machine"), tr("Back to main Page?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        ui->sw_assetManager->setCurrentIndex(static_cast<int>(AssetPages::ASSET_PAGE_MAIN));
    
}

void AssetManager::onPushNewMachine()
{
    _currentMachineId = 0;
    _currentMachineDetails = MachineInformation();
    ChangeTracker<MachineField>::SuspendGuard guard(_machineTracker);
    ClearFieldsForMachine();

    ui->pb_backMainPage->setEnabled(true);
    

    _isNewMachine = true;
    FillMachineListComboBox(GetSettings().getCompanyLocation());
    ResetDirtyStatus();


    ui->sw_assetManager->setCurrentIndex(static_cast<int>(AssetPages::ASSET_PAGE_LIST));

    UpdateMachineUiState();
}

