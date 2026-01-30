#include "CompanyContactData.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QVBoxLayout>

#include "CollapsibleSection.h"
#include "CompanyContactDataManager.h"
#include "CompanyContactProxyModel.h"
#include "CompanyContactTableModel.h"
#include "ui_CompanyContactData.h"

CompanyContactData::CompanyContactData(QWidget* parent) : QDialog(parent), ui(new Ui::CompanyContactDataClass())
{
    ui->setupUi(this);

    SetupAdvancedSearchUi();
    SetupTable();

    connect(ui->le_quickSearch, &QLineEdit::textChanged, this, [this](const QString&) { ApplyFilters(); });

    connect(ui->pb_search, &QPushButton::clicked, this, [this]() { ApplyFilters(); });

    connect(ui->pb_reset, &QPushButton::clicked, this,
            [this]()
            {
                ui->le_quickSearch->clear();
                _leCompanyName->clear();
                _leCustomerNumber->clear();
                _leCity->clear();
                _lePhone->clear();
                ApplyFilters();
            });

    connect(ui->tv_company, &QTableView::doubleClicked, this,
            [this](const QModelIndex&)
            {
                const auto* entry = GetSelectedEntry();
                if (entry == nullptr)
                    return;

                emit companySelected(entry->id, entry->companyName);

                accept();
            });

    connect(ui->tv_company, &QTableView::activated, this,
            [this](const QModelIndex&)
            {
                const auto* entry = GetSelectedEntry();
                if (entry == nullptr)
                    return;

                emit companySelected(entry->id, entry->companyName);
                accept();
            });
}

CompanyContactData::~CompanyContactData() { delete ui; }

void CompanyContactData::SetupAdvancedSearchUi()
{
    auto* advancedContent = new QWidget(ui->advancedPlaceholder);

    auto* form = new QFormLayout(advancedContent);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(6);

    _leCompanyName = new QLineEdit(advancedContent);
    _leCustomerNumber = new QLineEdit(advancedContent);
    _leCity = new QLineEdit(advancedContent);
    _lePhone = new QLineEdit(advancedContent);

    form->addRow(tr("Company name"), _leCompanyName);
    form->addRow(tr("Customer number"), _leCustomerNumber);
    form->addRow(tr("City"), _leCity);
    form->addRow(tr("Phone"), _lePhone);

    auto* advanced = new CollapsibleSection(tr("Advanced"), advancedContent, ui->advancedPlaceholder);

    auto* placeholderLayout = ui->advancedPlaceholder->layout();
    placeholderLayout->setContentsMargins(0, 0, 0, 0);
    placeholderLayout->addWidget(advanced);

    connect(_leCompanyName, &QLineEdit::textChanged, this, &CompanyContactData::OnAdvancedFilterChanged);
    connect(_leCustomerNumber, &QLineEdit::textChanged, this, &CompanyContactData::OnAdvancedFilterChanged);
    connect(_leCity, &QLineEdit::textChanged, this, &CompanyContactData::OnAdvancedFilterChanged);
    connect(_lePhone, &QLineEdit::textChanged, this, &CompanyContactData::OnAdvancedFilterChanged);
}

void CompanyContactData::SetupTable()
{
    _contactModel = new CompanyContactTableModel(this);
    _proxyModel = new CompanyContactProxyModel(this);

    _proxyModel->setSourceModel(_contactModel);

    ui->tv_company->setModel(_proxyModel);
    ui->tv_company->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tv_company->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tv_company->setSortingEnabled(true);
    ui->tv_company->setAlternatingRowColors(true);
    ui->tv_company->horizontalHeader()->setStretchLastSection(true);

    ui->tv_company->sortByColumn(static_cast<int>(CompanyContactTableModel::Column::CompanyName), Qt::AscendingOrder);

    ReloadData(false);

    ApplyFilters();
}

void CompanyContactData::ReloadData(bool includeDeleted)
{
    auto task = [this, includeDeleted]()
    {
        auto rows = CompanyContactDataManager::LoadAll(includeDeleted);

                QMetaObject::invokeMethod(
            this,
            [this, rows = std::move(rows)]()
            {
                _contactModel->setDataRows(rows);
                ApplyFilters();
            },
            Qt::QueuedConnection);        
    };
    
    Util::RunInThread(task, this);
}

void CompanyContactData::ApplyFilters()
{
    if (_proxyModel == nullptr)
        return;

    _proxyModel->setQuickFilterText(ui->le_quickSearch->text());
    _proxyModel->setAdvancedCompanyName(_leCompanyName->text());
    _proxyModel->setAdvancedCustomerNumber(_leCustomerNumber->text());
    _proxyModel->setAdvancedCity(_leCity->text());
    _proxyModel->setAdvancedPhone(_lePhone->text());
}

const CompanyContactEntry* CompanyContactData::GetSelectedEntry() const
{
    if (_proxyModel == nullptr || _contactModel == nullptr)
        return nullptr;

    const auto* sel = ui->tv_company->selectionModel();
    if (sel == nullptr || !sel->hasSelection())
        return nullptr;

    const QModelIndex proxyIndex = sel->currentIndex();
    if (!proxyIndex.isValid())
        return nullptr;

    const QModelIndex sourceIndex = _proxyModel->mapToSource(proxyIndex);
    return _contactModel->rowAt(sourceIndex.row());
}

void CompanyContactData::OnAdvancedFilterChanged(const QString&) { ApplyFilters(); }
