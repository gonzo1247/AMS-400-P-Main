
#include "ContractorWorkerData.h"
#include "ContractorWorkerDataManager.h"
#include "CompanyContactData.h"
#include "ContractorWorkerProxyModel.h"
#include "ContractorWorkerTableModel.h"
#include "DatabaseDefines.h"
#include "Util.h"

#include "ui_ContractorWorkerData.h"

ContractorWorkerData::ContractorWorkerData(QWidget* parent, bool onlySelect /*= false*/) : QWidget(parent), ui(new Ui::ContractorWorkerDataClass()),
_workerDataManager(std::make_unique<ContractorWorkerDataManager>()), _onlySelect(onlySelect)
{
	ui->setupUi(this);

    _companyContactData = new CompanyContactData(this);
    _contractorModel = new ContractorWorkerTableModel(this);
    _contractorProxy = new ContractorWorkerProxyModel(this);

    connect(_companyContactData, &CompanyContactData::companySelected, this,
            [this](std::uint32_t id, const QString& name)
            {
                SetCompanyData(id, name);
            });

    connect(ui->pb_selectCompany, &QPushButton::clicked, this, &ContractorWorkerData::onPushOpenContactData);
    connect(ui->le_companyName, &QLineEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);
    connect(ui->le_firstName, &QLineEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);
    connect(ui->le_lastName, &QLineEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);
    connect(ui->le_phone, &QLineEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);
    connect(ui->le_email, &QLineEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);
    connect(ui->te_note, &QTextEdit::textChanged, this, &ContractorWorkerData::UpdateSaveButtonState);

    connect(ui->pb_save, &QPushButton::clicked, this, &ContractorWorkerData::onPushSave);
    connect(ui->pb_back, &QPushButton::clicked, this, &ContractorWorkerData::onPushBackSearch);
    connect(ui->pb_delete, &QPushButton::clicked, this, &ContractorWorkerData::onPushDeleteWorker);
    UpdateSaveButtonState();

    _contractorProxy->setSourceModel(_contractorModel);

    ui->tv_worker->setModel(_contractorProxy);
    ui->tv_worker->setSortingEnabled(true);
    SetupWorkerTableHeader();
    LoadTable();
    ui->tv_worker->sortByColumn(0, Qt::AscendingOrder);

    connect(ui->le_quickSearch, &QLineEdit::textChanged, this,
            [this](const QString& t) { _contractorProxy->setQuickFilterText(t); });

    connect(ui->tv_worker, &QTableView::doubleClicked, this,
            [this](const QModelIndex& proxyIndex)
            {
                const QModelIndex sourceIndex = _contractorProxy->mapToSource(proxyIndex);
                const std::uint32_t id = _contractorModel->getWorkerId(sourceIndex.row());

                if (!_onlySelect)
                    LoadWorkerToEdit(id);
                else
                    LoadWorkerForSignal(id);
            });

    if (_onlySelect)
    {
        ui->pb_new->setEnabled(false);
        ui->pb_delete->setEnabled(false);
    }

    connect(ui->pb_new, &QPushButton::clicked, this, &ContractorWorkerData::onPushNewContractor);
}

ContractorWorkerData::~ContractorWorkerData()
{
	delete ui;
    delete _companyContactData;
}

void ContractorWorkerData::SetCompanyData(std::uint32_t id, const QString& name)
{
    _companyID = id;
    ui->le_companyName->setText(name);
}

bool ContractorWorkerData::HasValidRequiredFields() const
{
    if (ui->le_companyName->text().trimmed().isEmpty())
        return false;

    if (ui->le_firstName->text().trimmed().isEmpty() && ui->le_lastName->text().trimmed().isEmpty())
        return false;

    return true;
}

void ContractorWorkerData::UpdateSaveButtonState()
{
    ui->pb_save->setEnabled(HasValidRequiredFields() && HasChanges());
}

bool ContractorWorkerData::ConfirmMissingContactInfo()
{
    const bool hasPhone = !ui->le_phone->text().trimmed().isEmpty();
    const bool hasMail = !ui->le_email->text().trimmed().isEmpty();

    if (hasPhone || hasMail)
        return true;

    return QMessageBox::warning(this, tr("Missing contact information"),
                                tr("No phone number or e-mail address is set.\nDo you want to continue?"),
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

ContractorWorkerDataDB ContractorWorkerData::CollectWorkerData()
{
    ContractorWorkerDataDB data;
    data.id = _editWorkerID;
    data.companyID = _companyID;
    data.companyName = ui->le_companyName->text().trimmed().toStdString();
    data.firstName = ui->le_firstName->text().trimmed().toStdString();
    data.lastName = ui->le_lastName->text().trimmed().toStdString();
    data.phone = ui->le_phone->text().trimmed().toStdString();
    data.email = ui->le_email->text().trimmed().toStdString();
    data.note = ui->te_note->toPlainText().trimmed().toStdString();
    data.isActive = true /*ui->cb_active->isChecked()*/;
    return data;
}

void ContractorWorkerData::LoadTable()
{
    auto task = [this]()
    {
        auto rows = _workerDataManager->GetAllWorkers();
                QMetaObject::invokeMethod(
            this,
            [this, rows = std::move(rows)]()
            {
                _contractorModel->setDataRows(rows);
            },
            Qt::QueuedConnection);        
    };

    Util::RunInThread(task, this);
}

void ContractorWorkerData::LoadWorkerToEdit(std::uint32_t id)
{
    SetEditMode(true, id);

    auto worker = _workerDataManager->GetWorkerByID(id);

    QSignalBlocker b1(ui->le_companyName);
    QSignalBlocker b2(ui->le_firstName);
    QSignalBlocker b3(ui->le_lastName);
    QSignalBlocker b4(ui->le_phone);
    QSignalBlocker b5(ui->le_email);
    QSignalBlocker b6(ui->te_note);

    _companyID = worker.companyID;
    ui->le_companyName->setText(QString::fromStdString(worker.companyName));
    ui->le_firstName->setText(QString::fromStdString(worker.firstName));
    ui->le_lastName->setText(QString::fromStdString(worker.lastName));
    ui->le_phone->setText(QString::fromStdString(worker.phone));
    ui->le_email->setText(QString::fromStdString(worker.email));
    ui->te_note->setPlainText(QString::fromStdString(worker.note));
 //   ui->cb_active->setChecked(worker.isActive);
    

    SnapshotOriginal(worker);
    UpdateSaveButtonState();

    ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Edit));
}

void ContractorWorkerData::LoadWorkerForSignal(std::uint32_t id)
{
    auto worker = _workerDataManager->GetWorkerByID(id);

    emit SendWorkerData(worker.id, worker.firstName, worker.lastName, worker.phone, worker.companyName);
}

bool ContractorWorkerData::HasChanges()
{
    if (!_originalData.has_value())
        return true;  // New worker / no baseline

    const ContractorWorkerDataDB current = CollectWorkerData();
    const ContractorWorkerDataDB& old = *_originalData;

    return current.companyID != old.companyID || current.companyName != old.companyName ||
           current.firstName != old.firstName || current.lastName != old.lastName || current.phone != old.phone ||
           current.email != old.email || current.note != old.note;
}

void ContractorWorkerData::SnapshotOriginal(const ContractorWorkerDataDB& data)
{
    _originalData = data;
}

bool ContractorWorkerData::ConfirmDiscardChanges()
{
    if (!HasChanges())
        return true;

    return QMessageBox::question(this, tr("Discard changes?"),
                                 tr("You have unsaved changes.\nDo you want to discard them?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

bool ContractorWorkerData::TryLeaveEditMode()
{
    if (!ConfirmDiscardChanges())
        return false;

    ResetEditForm();
    SetEditMode(false);
    ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Search));
    return true;
}

void ContractorWorkerData::SetEditMode(bool editMode, std::uint32_t workerId /*= 0*/)
{
    _editMode = editMode;
    _editWorkerID = editMode ? workerId : 0;

    ui->pb_save->setText(_editMode ? tr("Update") : tr("Save"));
}

void ContractorWorkerData::ResetEditForm()
{
    _companyID = 0;
    _originalData.reset();

    ui->le_companyName->clear();
    ui->le_firstName->clear();
    ui->le_lastName->clear();
    ui->le_phone->clear();
    ui->le_email->clear();
    ui->te_note->clear();

    UpdateSaveButtonState();
}

void ContractorWorkerData::SetupWorkerTableHeader()
{
    if (!ui->tv_worker || !ui->tv_worker->model())
        return;

    QHeaderView* hHeader = ui->tv_worker->horizontalHeader();
    QHeaderView* vHeader = ui->tv_worker->verticalHeader();

    vHeader->setVisible(false);

    hHeader->setStretchLastSection(false);
    hHeader->setSectionsMovable(true);
    hHeader->setHighlightSections(false);

    // Default mode
    hHeader->setSectionResizeMode(QHeaderView::Interactive);

    // Column specific behavior
    hHeader->setSectionResizeMode(0, QHeaderView::Stretch);           // Company
    hHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // First name
    hHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // Last name
    hHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Phone
    hHeader->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // E-Mail
    hHeader->setSectionResizeMode(5, QHeaderView::Stretch);           // Note
    hHeader->setSectionResizeMode(6, QHeaderView::ResizeToContents);  // Active

    ui->tv_worker->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tv_worker->setSelectionMode(QAbstractItemView::SingleSelection);
}

void ContractorWorkerData::onPushOpenContactData()
{
    if (!_companyContactData)
        _companyContactData = new CompanyContactData(this);

    _companyContactData->exec();
}

void ContractorWorkerData::onPushSave()
{
    if (!HasValidRequiredFields())
        return;

    if (!ConfirmMissingContactInfo())
        return;

    bool success = false;

    if (_editMode)
    {
        success = _workerDataManager->UpdateWorker(CollectWorkerData());
    }
    else
    {
        success = _workerDataManager->SaveNewWorker(CollectWorkerData());
    }

    if (success)
    {
        QMessageBox::information(this, tr("Save successful"),
                                 _editMode ? tr("The contractor worker has been updated successfully.")
                                           : tr("The contractor worker has been saved successfully."));

        ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Search));
    }
    else
    {
        QMessageBox::critical(this, tr("Save failed"), tr("An error occurred while saving the contractor worker."));
    }

    ResetEditForm();
    SetEditMode(false);
    ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Search));
    LoadTable();
}

void ContractorWorkerData::onPushNewContractor()
{
    ResetEditForm();
    SetEditMode(false);

    ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Edit));
}

void ContractorWorkerData::onPushBackSearch()
{
    if (!TryLeaveEditMode())
        return;

    ui->stackedWidget->setCurrentIndex(static_cast<int>(Mode::Search));
}

void ContractorWorkerData::onPushDeleteWorker()
{
    QItemSelectionModel* sel = ui->tv_worker->selectionModel();
    if (!sel || !sel->hasSelection())
        return;

    const QModelIndexList selectedRows = sel->selectedRows();
    if (selectedRows.isEmpty())
        return;

    const QModelIndex proxyIndex = selectedRows.first();
    if (!proxyIndex.isValid())
        return;

    const QModelIndex sourceIndex = _contractorProxy->mapToSource(proxyIndex);
    if (!sourceIndex.isValid())
        return;

    const std::uint32_t workerId = _contractorModel->getWorkerId(sourceIndex.row());

    if (QMessageBox::question(this, tr("Delete worker"), tr("Do you really want to delete the selected worker?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (!_workerDataManager->SoftDeleteWorker(workerId))
    {
        QMessageBox::critical(this, tr("Delete failed"), tr("An error occurred while deleting the worker."));
        return;
    }

    LoadTable();
}


