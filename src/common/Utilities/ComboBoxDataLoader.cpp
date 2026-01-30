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

#include "ComboBoxDataLoader.h"

#include "ConnectionGuard.h"
#include "DatabaseConnection.h"
#include "ConnectionPool.h"

std::map<CompanyLocations, std::vector<StorageRoom>> ComboBoxDataLoader::LoadStorageRooms()
{
	std::map<CompanyLocations, std::vector<StorageRoom>> rooms;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	const StorageRoom noneRoom(0, "None");
	rooms[CompanyLocations::CL_NONE].push_back(noneRoom);

	for (int i = static_cast<int>(CompanyLocations::CL_BBG); i <= static_cast<int>(CompanyLocations::CL_FRN); i <<= 1)
	{
		auto location = static_cast<CompanyLocations>(i);

		auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_SR_SELECT_ALL_ROOMS);
		stmt->SetUInt(1, i);

		auto result = guard->ExecutePreparedSelect(*stmt);

		if (result.IsValid())
		{
			while (result.NextRow())
			{
                Field* fields = result.Fetch();

				StorageRoom room;
				room.id = fields[0].GetInt32(); // result->getInt(1);
				room.name = fields[1].ToString(); // result->getString(2);
				room.location = location;

				rooms[location].push_back(room);
			}
		}
	}

	for (auto& [loc, vec] : rooms)
		std::sort(vec.begin(), vec.end());

	return rooms;
}

std::map<CompanyLocations, std::vector<CompanyLocationData>> ComboBoxDataLoader::LoadCompanyLocations()
{
	std::map<CompanyLocations, std::vector<CompanyLocationData>> locations;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_CL_SELECT_ALL_COMPANY_LOCATIONS);
	auto result = guard->ExecutePreparedSelect(*stmt);

	if (result.IsValid())
	{
		while (result.NextRow())
		{
			Field* fields = result.Fetch();
			CompanyLocationData data;
			data.id = fields[0].GetInt32();
			data.locAbbr = fields[1].ToString(); // result->getString(2);
			data.FullName = fields[2].ToString(); // result->getString(3);

			auto location = static_cast<CompanyLocations>(data.id);
			locations[location].push_back(data);
		}
	}

	for (auto& [loc, vec] : locations)
		std::sort(vec.begin(), vec.end());

	return locations;
}

void ComboBoxDataLoader::FillRoomCombo(QComboBox* comboBox, CompanyLocations location, const std::map<CompanyLocations, std::vector<StorageRoom>>& roomMap)
{
	if (!comboBox)
		return;

	comboBox->clear();
	QStringList itemList;

	const auto it = roomMap.find(location);
	if (it != roomMap.end())
	{
		for (const auto& room : it->second)
		{
			comboBox->addItem(QString::fromStdString(room.name), room.id);
			itemList.append(QString::fromStdString(room.name));
		}
	}

	auto* completer = new QCompleter(itemList, comboBox);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	comboBox->setCompleter(completer);
}

void ComboBoxDataLoader::FillCompanyLocationCombo(QComboBox* comboBox, const std::map<CompanyLocations, std::vector<CompanyLocationData>>& locationMap)
{
	if (!comboBox)
		return;

	comboBox->clear();
	QStringList itemList;

	for (const auto& [loc, vec] : locationMap)
	{
		for (const auto& entry : vec)
		{
			comboBox->addItem(QString::fromStdString(entry.FullName), entry.id);
			itemList.append(QString::fromStdString(entry.FullName));
		}
	}

	auto* completer = new QCompleter(itemList, comboBox);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	comboBox->setCompleter(completer);
}

std::map<CompanyLocations, std::vector<StorageRoom>> ComboBoxDataLoader::ReloadAndFillRoomCombo(QComboBox* comboBox, CompanyLocations location)
{
	auto rooms = LoadStorageRooms();
	FillRoomCombo(comboBox, location, rooms);
	return rooms;
}

std::map<CompanyLocations, std::vector<CompanyLocationData>> ComboBoxDataLoader::ReloadAndFillCompanyLocationCombo(QComboBox* comboBox)
{
	auto locations = LoadCompanyLocations();
	FillCompanyLocationCombo(comboBox, locations);
	return locations;
}
