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

#include "LogDataManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "UserManagement.h"

LogDataManager::LogDataManager() : _timeMgr(std::make_unique<TimeMgr>())
{
}

void LogDataManager::WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::string originalData /*= ""*/, std::string changedData /*= ""*/)
{
	_WriteDataToDatabase(flags, internalID, originalData, changedData);
}

void LogDataManager::WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::uint32_t originalData, std::string changedData)
{
	_WriteDataToDatabase(flags, internalID, Util::ConvertUint32ToString(originalData), changedData);
}

void LogDataManager::WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::uint32_t originalData, std::uint32_t changedData)
{
	_WriteDataToDatabase(flags, internalID, Util::ConvertUint32ToString(originalData), Util::ConvertUint32ToString(changedData));
}

void LogDataManager::WriteLogData(LogFilterFlags flags, std::uint32_t internalID, std::string originalData, std::uint32_t changedData)
{
	_WriteDataToDatabase(flags, internalID, originalData, Util::ConvertUint32ToString(changedData));
}

void LogDataManager::CleanupLogData()
{
	ConnectionGuardIMS _connection(database::ConnectionType::Sync);

	// DELETE FROM logs WHERE LogDate <= ?
	auto cleanupStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_LOG_DATA_DELETE_OLDER_ENTRIES);
	std::uint64_t cleanupTime = _timeMgr->ReturnPastOfDays(180);
	cleanupStmt->SetUInt64(1, cleanupTime);

	_connection->ExecutePreparedDelete(*cleanupStmt);
	LOG_SQL("LogDataManager::CleanupLogData() cleanup data they are older than: {} ", cleanupTime);
}

// private Member
void LogDataManager::_WriteDataToDatabase(LogFilterFlags flags, std::uint32_t internalID, std::string originalData, std::string changedData)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// DB_LOG_DATA_INSERT_NEW_LOG, "INSERT INTO logs (LogFlag, LogDate, InternalID, OriginalData, ChangedData, UserName) VALUES (?, ?, ?, ?, ?, ?)
	auto logInsert = _connection->GetPreparedStatement(IMSPreparedStatement::DB_LOG_DATA_INSERT_NEW_LOG);

	logInsert->SetUInt(1, static_cast<std::uint32_t>(flags));
	logInsert->SetUInt64(2, _timeMgr->GetUnixTime());
	logInsert->SetUInt(3, internalID);
	logInsert->SetString(4, originalData);
	logInsert->SetString(5, changedData);
	logInsert->SetString(6, GetUser().GetUserName());

	if (!_connection->ExecutePreparedInsert(*logInsert))
		LOG_SQL("Can not insert Log Data. Check it!");
}
