#pragma once
#include "DatabaseDefines.h"
#include "Define.h"

class CallerManager
{
public:
	CallerManager() = default;
    ~CallerManager() = default;

	bool LoadCallerData();
	
	bool AddCaller(const CallerInformation& caller);

	bool EditCaller(const CallerInformation& caller);
    bool RemoveCaller(std::uint64_t callerID);

	std::unordered_map<std::uint64_t, CallerInformation> GetCallerMap() const;

private:
    std::uint64_t GetNewCallerID(const CallerInformation& info);



	std::unordered_map<std::uint64_t, CallerInformation> callerMap;
};

