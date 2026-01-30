#pragma once
#include <map>
#include <QComboBox>

#include "DatabaseDefines.h"
#include "SharedDefines.h"

class CompanyLocationHandler
{
   public:
    // Singleton accessor
    static CompanyLocationHandler& instance()
    {
        static CompanyLocationHandler instance;
        return instance;
    }

    // Delete copy/move operations
    CompanyLocationHandler(const CompanyLocationHandler&) = delete;
    CompanyLocationHandler& operator=(const CompanyLocationHandler&) = delete;
    CompanyLocationHandler(CompanyLocationHandler&&) = delete;
    CompanyLocationHandler& operator=(CompanyLocationHandler&&) = delete;

   public:
    CompanyLocationHandler() = default;
    ~CompanyLocationHandler() = default;

    void Initialize();

    void FillComboBoxWithData(QComboBox* comboBox);
    std::string GetLocationNameById(std::uint32_t id);

private:
    void LoadCompanyData();

    std::map<std::uint32_t, CompanyLocationDatabase> _companyData;
};

