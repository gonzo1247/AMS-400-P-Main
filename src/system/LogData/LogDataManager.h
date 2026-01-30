/*
 * Copyright (C) 2023 - 2025 Severin Weitz, Lukas Winter | WeWi-Systems
 *
 * This file is part of the Inventory Management System project,
 * licensed under the GNU Lesser General Public License (LGPL) v3.
 *
 * For more details, see the full license header in main.cpp or
 * InventoryManagementSystem.cpp and the LICENSE.txt file.
 *
 * Author: Severin Weitz, Lukas Winter
 * Date: [07.10.2024]
 */

#pragma once

#include "SharedDefines.h"
#include "Util.h"
#include "TimeMgr.h"

class LogDataManager
{
public:
	LogDataManager();

	void WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::string originalData = "", std::string changedData = "");
	void WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::uint32_t originalData, std::string changedData);
	void WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::uint32_t originalData, std::uint32_t changedData);
	void WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::string originalData, std::uint32_t changedData);

	void CleanupLogData(); // delete all logs there older than 30 days?

private:
	void _WriteDataToDatabase(LogFilterFlags flags, std::uint32_t internalID, std::string originalData, std::string changedData);
	
	std::unique_ptr<TimeMgr> _timeMgr;
};

