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
#include <QComboBox>
#include <QCompleter>
#include <QStringList>

#include "SharedDefines.h"

class ComboBoxDataLoader
{
public:
	static std::map<CompanyLocations, std::vector<StorageRoom>> LoadStorageRooms();
	static std::map<CompanyLocations, std::vector<CompanyLocationData>> LoadCompanyLocations();

	static void FillRoomCombo(QComboBox* comboBox, CompanyLocations location,
		const std::map<CompanyLocations, std::vector<StorageRoom>>& roomMap);

	static void FillCompanyLocationCombo(QComboBox* comboBox,
		const std::map<CompanyLocations, std::vector<CompanyLocationData>>& locationMap);

	static std::map<CompanyLocations, std::vector<StorageRoom>> ReloadAndFillRoomCombo(
		QComboBox* comboBox,
		CompanyLocations location);

	static std::map<CompanyLocations, std::vector<CompanyLocationData>> ReloadAndFillCompanyLocationCombo(
		QComboBox* comboBox);
};
