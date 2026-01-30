#pragma once
#include "DatabaseDefines.h"

class EmployeeManager
{
public:
	EmployeeManager() = default;
    ~EmployeeManager() = default;

	void LoadEmployeeData();
    std::unordered_map<std::uint32_t, EmployeeInformation> GetEmployeeMap() const;
	bool AddNewEmployee(const EmployeeInformation& info);
    bool UpdateEmployee(const EmployeeInformation& info);
	bool DeleteEmployee(const std::uint32_t& employeeID);

	// for ticket details
	std::vector<EmployeeInformation> LoadEmployeeDataForDetails(std::uint64_t ticketID);

	// ticket assignment
    void AddEmployeeAssignment(std::uint64_t ticketID, std::uint32_t employeeID, const std::string& comment = "");
    void RemoveEmployeeAssignment(std::uint64_t ticketID, std::uint32_t employeeID, const std::string& comment = "");

private:
    std::uint32_t GetLastInsertEmployeeID(const EmployeeInformation& info);

	std::unordered_map<std::uint32_t, EmployeeInformation> employeeMap;

};

