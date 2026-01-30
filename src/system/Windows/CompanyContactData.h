#pragma once

#include <QDialog>

struct CompanyContactEntry;
QT_BEGIN_NAMESPACE
namespace Ui { class CompanyContactDataClass; };
QT_END_NAMESPACE

class QLineEdit;

class CompanyContactProxyModel;
class CompanyContactTableModel;

class CompanyContactData : public QDialog
{
	Q_OBJECT

public:
	explicit CompanyContactData(QWidget *parent = nullptr);
	~CompanyContactData() override;

private:
    void SetupAdvancedSearchUi();
    void SetupTable();
    void ReloadData(bool includeDeleted);
    void ApplyFilters();
    const CompanyContactEntry* GetSelectedEntry() const;

    // Advanced search fields
    QLineEdit* _leCompanyName = nullptr;
    QLineEdit* _leCustomerNumber = nullptr;
    QLineEdit* _leCity = nullptr;
    QLineEdit* _lePhone = nullptr;

    CompanyContactTableModel* _contactModel = nullptr;
    CompanyContactProxyModel* _proxyModel = nullptr;


	Ui::CompanyContactDataClass *ui;

private slots:
    void OnAdvancedFilterChanged(const QString& text);

signals:
    void companySelected(std::uint32_t id, const QString& companyName);
};

