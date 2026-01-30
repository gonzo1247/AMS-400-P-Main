#include "pch.h"
#include "ContractVisit.h"
#include "ContractorWorkerData.h"
#include "CompanyContactData.h"
#include "ContractorVisitStatusManager.h"
#include "ContractorVisitStatusModel.h"

#include "Util.h"

ContractVisit::ContractVisit(QWidget *parent) : QWidget(parent), ui(new Ui::ContractVisitClass())
{
	ui->setupUi(this);

    _visitModel = new ContractorVisitModel(ContractorVisitModel::ViewMode::Admin, this);
    _visitProxy = new ContractorVisitProxyModel(this);
    _visitProxy->setSourceModel(_visitModel);

    ui->tv_visits->setModel(_visitProxy);
    ui->tv_visits->setSortingEnabled(true);

    _assignedWorkerModel = new ContractorVisitWorkerModel(this);
    ui->tv_visitWorker->setModel(_assignedWorkerModel);
    ui->tv_visitWorker->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tv_visitWorker->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tv_visitWorker->horizontalHeader()->setStretchLastSection(true);

    _workerDataWidget = new ContractorWorkerData(nullptr, true);
    _workerDataWidget->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(_workerDataWidget, &ContractorWorkerData::SendWorkerData, this, &ContractVisit::onReceiveWorkerData);

    _companyContactWidget = new CompanyContactData(this);
    connect(_companyContactWidget, &CompanyContactData::companySelected, this, &ContractVisit::onReceiveCompanyData);

    connect(ui->le_quickSearch, &QLineEdit::textChanged, this,
            [this](const QString& t) { _visitProxy->setQuickFilterText(t); });

    connect(ui->pb_addVisit, &QPushButton::clicked, this, &ContractVisit::onPushAddNew);

    connect(ui->tv_visits, &QTableView::doubleClicked, this,
            [this](const QModelIndex& proxyIndex)
            {
                if (!proxyIndex.isValid())
                    return;

                const QModelIndex sourceIndex = _visitProxy->mapToSource(proxyIndex);
                if (!sourceIndex.isValid())
                    return;

                const std::uint64_t visitId =
                    _visitModel->data(sourceIndex, ContractorVisitModel::RoleVisitId).toULongLong();

                LoadVisitToEdit(visitId);
            });

    connect(ui->pb_selectWorker, &QPushButton::clicked, this, &ContractVisit::onPushSelectWorker);

    {
        ui->pb_addWorker->setEnabled(CheckWorkerData());
        connect(ui->le_workerFirstName, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonsManuallyWorkerAdd);
        connect(ui->le_workerLastName, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonsManuallyWorkerAdd);
        connect(ui->le_phone, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonsManuallyWorkerAdd);
        connect(ui->le_companyName, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonsManuallyWorkerAdd);
    }

    connect(ui->pb_addWorker, &QPushButton::clicked, this, &ContractVisit::onPushNewWorker);
    connect(ui->pb_selectCompany, &QPushButton::clicked, this, &ContractVisit::onPushSelectCompany);
    connect(ui->dte_arrivalAt, &QDateTimeEdit::dateTimeChanged, this, &ContractVisit::onArrivalAtTimeChanged);
    connect(ui->dte_departureAt, &QDateTimeEdit::dateTimeChanged, this, &ContractVisit::onDepartureAtTimeChanged);

    {
        connect(ui->le_contactPerson, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->le_location, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->le_activity, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->te_note, &QTextEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->le_contactPhone, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->le_reachableNote, &QLineEdit::textChanged, this, &ContractVisit::UpdateButtonSave);
        connect(ui->cb_status, &QComboBox::currentIndexChanged, this, &ContractVisit::UpdateButtonSave);
    }

    connect(ui->pb_saveVisit, &QPushButton::clicked, this, &ContractVisit::onPushSaveVisit);
    connect(ui->pb_setArrivalAt, &QPushButton::clicked, this, &ContractVisit::onPushSetArrivalTime);
    connect(ui->pb_setDepartureAt, &QPushButton::clicked, this, &ContractVisit::onPushSetDepartureTime);

    connect(ui->pb_removeWorker, &QPushButton::clicked, this, &ContractVisit::onPushRemoveWorker);

    connect(ui->tv_visitWorker->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            &ContractVisit::onSelectedWorkerRowChanged);
    connect(ui->pb_back, &QPushButton::clicked, this, &ContractVisit::onPushBack);

    LoadStatusComboBox();
}

ContractVisit::~ContractVisit()
{
	delete ui;
}

void ContractVisit::LoadTableData()
{
    LoadVisitTable();
}

void ContractVisit::LoadVisitTable()
{
    if (!_visitMgr)
        _visitMgr = std::make_unique<ContractorVisitManager>();

    auto task = [this]()
    {
        _visitMgr->LoadContractorVisitData();
        auto rows = _visitMgr->GetContractorVisits();
        QMetaObject::invokeMethod(
            this,
            [this, rows = std::move(rows)]()
            {
                _visitModel->setRows(rows);
            },
            Qt::QueuedConnection);
    };

    Util::RunInThread(task, this);
}

void ContractVisit::LoadVisitToEdit(std::uint64_t id)
{
    if (!_visitMgr)
        _visitMgr = std::make_unique<ContractorVisitManager>();

    auto visit = _visitMgr->GetContractorVisitByID(id);

    _editVisitInfo = visit;
    _originalVisitInfo = visit;

    const QSignalBlocker bArrival(ui->dte_arrivalAt);
    const QSignalBlocker bDeparture(ui->dte_departureAt);
    const QSignalBlocker bContactPerson(ui->le_contactPerson);
    const QSignalBlocker bActivity(ui->le_activity);
    const QSignalBlocker bLocation(ui->le_location);
    const QSignalBlocker bContactPhone(ui->le_contactPhone);
    const QSignalBlocker bNote(ui->te_note);
    const QSignalBlocker bReachableNote(ui->le_reachableNote);
    const QSignalBlocker bStatus(ui->cb_status);

    ui->dte_arrivalAt->setDateTime(Util::ConvertToQDateTime(visit.arrival_at));
    if (visit.departure_at.time_since_epoch().count() != 0)
        ui->dte_departureAt->setDateTime(Util::ConvertToQDateTime(visit.departure_at));
    else
        ui->dte_departureAt->setDateTime(QDateTime());

    ui->le_contactPerson->setText(QString::fromStdString(visit.contactPerson));
    ui->le_activity->setText(QString::fromStdString(visit.activity));
    ui->le_location->setText(QString::fromStdString(visit.location));
    ui->le_contactPhone->setText(QString::fromStdString(visit.contactPhone));
    ui->te_note->setText(QString::fromStdString(visit.note));
    ui->le_reachableNote->setText(QString::fromStdString(visit.contactNote));
        
    SetStatus(visit.status);

    auto worker = _visitMgr->GetWorkersForVisit(id);
    _assignedWorkerModel->setRows(worker);
    _assignedWorkers = worker;

    for (const auto& w : worker)
        _assignedWorkerIDs.insert(w.workerId);

    ui->pb_saveVisit->setText(tr("Update Visit"));

    ui->stackedWidget->setCurrentIndex(static_cast<int>(Pages::VisitDetails));

    _isEditMode = true;
}

void ContractVisit::RemoveWorker(std::uint64_t workerId)
{
    _assignedWorkerIDs.erase(workerId);

    std::erase_if(_assignedWorkers, [workerId](const ContractorVisitWorkerInfo& w) { return w.workerId == workerId; });

    if (!_workersToAdd.contains(workerId))
        _workersToRemove.insert(workerId);

    _assignedWorkerModel->setRows(_assignedWorkers);
}

bool ContractVisit::CheckWorkerData()
{
    const bool hasFirstName = ui->le_workerFirstName->text().trimmed().size() >= 3;
    const bool hasLastName = ui->le_workerLastName->text().trimmed().size() >= 3;
    const bool hasPhone = ui->le_phone->text().trimmed().size() >= 3;
    const bool hasCompany = ui->le_companyName->text().trimmed().size() >= 3;

    const bool hasName = hasFirstName && hasLastName;
    const bool hasContact = hasPhone || hasCompany;

    return hasName && hasContact;
}

void ContractVisit::UpdateButtonsManuallyWorkerAdd()
{
    ui->pb_addWorker->setEnabled(CheckWorkerData());
}

void ContractVisit::UpdateButtonSave()
{
    ui->pb_saveVisit->setEnabled(HasChanges());
}

void ContractVisit::CollectEditChanges()
{
    _editVisitInfo.contactPerson = ui->le_contactPerson->text().trimmed().toStdString();
    _editVisitInfo.location = ui->le_location->text().trimmed().toStdString();
    _editVisitInfo.activity = ui->le_activity->text().trimmed().toStdString();
    _editVisitInfo.note = ui->te_note->toPlainText().trimmed().toStdString();
    _editVisitInfo.contactPhone = ui->le_contactPhone->text().trimmed().toStdString();
    _editVisitInfo.contactNote = ui->le_reachableNote->text().trimmed().toStdString();
    _editVisitInfo.status = static_cast<std::uint8_t>(ui->cb_status->currentIndex());
    _editVisitInfo.contractorCompany = _selectCompanyName;
    _editVisitInfo.contractorCompanyID = _selectedCompanyId;
    std::uint8_t statusId = static_cast<std::uint8_t>(ui->cb_status->currentData(ContractorVisitStatusModel::IdRole).toUInt());
    _editVisitInfo.status = statusId;

}

bool ContractVisit::HasChanges()
{
    CollectEditChanges();

    return _editVisitInfo.contactPerson != _originalVisitInfo.contactPerson             ||
           _editVisitInfo.location != _originalVisitInfo.location                       ||
           _editVisitInfo.activity != _originalVisitInfo.activity                       ||
           _editVisitInfo.note != _originalVisitInfo.note                               ||
           _editVisitInfo.contractorCompany != _originalVisitInfo.contractorCompany     ||
           _editVisitInfo.contractorCompanyID != _originalVisitInfo.contractorCompanyID ||
           _editVisitInfo.status != _originalVisitInfo.status                           ||
           !_workersToAdd.empty()                                                       ||
           !_workersToRemove.empty();
}

void ContractVisit::LoadStatusComboBox()
{
    _statusModel = new ContractorVisitStatusModel(this);

    auto statuses = ContractorVisitStatusManager::GetAll();
    _statusModel->setRows(std::move(statuses));

    ui->cb_status->setModel(_statusModel);
}

void ContractVisit::SetStatus(std::uint16_t statusId)
{
    if (!ui->cb_status->model())
        return;

    const int idx = ui->cb_status->findData(static_cast<quint16>(statusId), ContractorVisitStatusModel::IdRole);

    if (idx >= 0)
        ui->cb_status->setCurrentIndex(idx);
    else
        ui->cb_status->setCurrentIndex(-1);  // Not found -> empty selection
}

void ContractVisit::onPushAddNew()
{
    _editVisitInfo = ContractorVisitInformation{};
    _selectedCompanyId = 0;
    _selectCompanyName.clear();

    _workersToAdd.clear();
    _workersToRemove.clear();
    _assignedWorkers.clear();

    SetStatus(0);

    ui->pb_saveVisit->setText(tr("Save Visit"));

    ui->stackedWidget->setCurrentIndex(static_cast<int>(Pages::VisitDetails));
}

void ContractVisit::onReceiveWorkerData(std::uint32_t workerID, std::string firstName, std::string lastName, std::string phone, std::string companyName)
{

    if (_assignedWorkerIDs.contains(workerID))
    {
        QMessageBox::information(this, tr("Information"), tr("Worker is already assigned."));
        return;
    }

    ContractorVisitWorkerInfo w;
    w.workerId = workerID;
    w.firstName = std::move(firstName);
    w.lastName = std::move(lastName);
    w.phone = std::move(phone);
    w.company = std::move(companyName);
    _selectCompanyName = w.company;

    _assignedWorkers.push_back(w);
    _assignedWorkerIDs.insert(workerID);
    _workersToAdd.insert(workerID);

    _assignedWorkerModel->setRows(_assignedWorkers);    
}

void ContractVisit::onPushSelectWorker()
{
    if (!_workerDataWidget)
        return;

    _workerDataWidget->show();
    _workerDataWidget->raise();
    _workerDataWidget->activateWindow();
}

void ContractVisit::onReceiveCompanyData(std::uint32_t id, const QString& companyName)
{
    ui->le_companyName->setText(companyName);
    _selectedCompanyId = id;
    _selectCompanyName = companyName.toStdString();
}

void ContractVisit::onPushSelectCompany()
{
    if (!_companyContactWidget)
        return;

    _companyContactWidget->setWindowModality(Qt::ApplicationModal);
    _companyContactWidget->show();
    _companyContactWidget->raise();
    _companyContactWidget->activateWindow();
}

void ContractVisit::onPushNewWorker()
{
    ContractorVisitWorkerInfo w{};

    w.firstName = ui->le_workerFirstName->text().trimmed().toStdString();
    w.lastName = ui->le_workerLastName->text().trimmed().toStdString();
    w.phone = ui->le_phone->text().trimmed().toStdString();
    w.company = ui->le_companyName->text().trimmed().toStdString();


    const bool newDBWorker = QMessageBox::question(this, tr("Add Worker to Database"),
                                                   tr("Should this worker be added to the database as a new worker?"),
                                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;

    if (newDBWorker)
    {
        if (!_visitMgr)
            _visitMgr = std::make_unique<ContractorVisitManager>();

        const std::uint64_t newWorkerId = _visitMgr->AddNewWorker(w, _selectedCompanyId);
        if (newWorkerId == 0)
        {
            QMessageBox::warning(this, tr("Error"), tr("Failed to add new worker to database."));
            return;
        }
        w.workerId = newWorkerId;
    }

    const auto isSameManual = [&w](const ContractorVisitWorkerInfo& x)
    {
        return x.workerId == 0 && x.firstName == w.firstName && x.lastName == w.lastName && x.company == w.company &&
               x.phone == w.phone;
    };

    if (std::ranges::any_of(_assignedWorkers, isSameManual))
    {
        QMessageBox::information(this, tr("Information"), tr("Worker is already assigned."));
        return;
    }

    _assignedWorkers.push_back(w);

    if (w.workerId != 0)
    {
        _assignedWorkerIDs.insert(w.workerId);
        _workersToAdd.insert(w.workerId);
    }

    _assignedWorkerModel->setRows(_assignedWorkers);

    ui->le_workerFirstName->clear();
    ui->le_workerLastName->clear();
    ui->le_phone->clear();
    ui->le_companyName->clear();
    UpdateButtonsManuallyWorkerAdd();
}

void ContractVisit::onArrivalAtTimeChanged(const QDateTime& datetime)
{
    _editVisitInfo.arrival_at = Util::ConvertFromQDateTime(datetime);
}

void ContractVisit::onDepartureAtTimeChanged(const QDateTime& datetime)
{
    _editVisitInfo.departure_at = Util::ConvertFromQDateTime(datetime);
}

void ContractVisit::onPushSaveVisit()
{
    if (_isEditMode)
    {
        if (!HasChanges())
        {
            QMessageBox::information(this, tr("Information"), tr("No changes to save."));
            return;
        }

        if (_visitMgr->UpdateVisit(_editVisitInfo, _workersToAdd, _workersToRemove))
        {
            QMessageBox::information(this, tr("Successfully Updated"), tr("The visit was successfully updated."));

            _originalVisitInfo = _editVisitInfo;
            _workersToAdd.clear();
            _workersToRemove.clear();

            UpdateButtonSave();
        }
        else
        {
            QMessageBox::critical(this, tr("Update failed"),
                                  tr("The visit update has failed.\nPlease check the logs for more information."));
        }
    }
    else
    {
        if (!HasChanges())
        {
            QMessageBox::information(this, tr("Information"), tr("No changes to save."));
            return;
        }

        if (_visitMgr->AddNewVisit(_editVisitInfo, _workersToAdd, _workersToRemove))
        {
            QMessageBox::information(this, tr("Successfully Added"), tr("The visit was successfully added."));
            _workersToAdd.clear();
            _workersToRemove.clear();
            LoadVisitTable();
            ui->stackedWidget->setCurrentIndex(static_cast<int>(Pages::VisitList));
        }
        else
        {
            QMessageBox::critical(this, tr("Add failed"),
                                  tr("The visit addition has failed.\nPlease check the logs for more information."));
        }
    }

}

void ContractVisit::onPushSetArrivalTime()
{
    ui->dte_arrivalAt->setDateTime(QDateTime::currentDateTime());
}

void ContractVisit::onPushSetDepartureTime()
{
    ui->dte_departureAt->setDateTime(QDateTime::currentDateTime());
}

void ContractVisit::onPushRemoveWorker()
{
    const QModelIndex current = ui->tv_visitWorker->currentIndex();
    if (!current.isValid())
        return;

    const QModelIndex firstNameIdx = current.siblingAtColumn(ContractorVisitWorkerModel::COL_FIRST_NAME);
    const QModelIndex lastNameIdx = current.siblingAtColumn(ContractorVisitWorkerModel::COL_LAST_NAME);

    const QString displayName =
        QString("%1 %2")
            .arg(firstNameIdx.data(Qt::DisplayRole).toString(), lastNameIdx.data(Qt::DisplayRole).toString())
            .trimmed();

    const std::uint64_t workerId =
        current.siblingAtColumn(0).data(ContractorVisitWorkerModel::RoleWorkerId).toULongLong();

    const auto answer =
        QMessageBox::question(this, tr("Remove Worker"), tr("Remove worker '%1' from this visit?").arg(displayName),
                              QMessageBox::Yes | QMessageBox::No);

    if (answer != QMessageBox::Yes)
        return;

    RemoveWorker(workerId);
    UpdateButtonSave();
}

void ContractVisit::onSelectedWorkerRowChanged(const QModelIndex& current, const QModelIndex&)
{
    _selectedWorkerId = 0;

    if (current.isValid())
    {
        const QModelIndex idIndex = current.siblingAtColumn(0);
        _selectedWorkerId = idIndex.data(ContractorVisitWorkerModel::RoleWorkerId).toULongLong();
    }

    ui->pb_removeWorker->setEnabled(_selectedWorkerId != 0);
}

void ContractVisit::onPushBack()
{
    ui->stackedWidget->setCurrentIndex(static_cast<int>(Pages::VisitList));
}
