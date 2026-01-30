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

#include <string>
#include <QDateTimeEdit>

#include "TranslateText.h"

class TimeMgr
{
public:
	TimeMgr();
	~TimeMgr();

	std::uint64_t GetUnixTime();
	time_t GetUnixTimeAsTime();

	std::uint64_t ReturnPastOfThirtyDays();
	std::uint64_t ReturnPastOfDays(std::uint32_t returnDays);

	std::uint64_t GetDTETimeAsUint(QDateTimeEdit& timeEdit);
	std::uint64_t GetDTETimeAsUint(const QDateTimeEdit& timeEdit);
	std::string GetTimeEditTimeAsString(QDateTimeEdit& timeEdit);
	std::string GetTimeEditTimeAsString(const QDateTimeEdit& timeEdit);

	std::string ConvertUnixTimeToString(time_t unixTime);
	std::string ConvertUnixTimeToStringWithDefaultValue(time_t unixTime);
	std::pair<std::string, std::string> ConvertUnixTimeToDataTimeEdit(time_t unixTime);

	QDateTime ConvertTimeToQDataTime(std::pair<std::string, std::string> timeToConvert);

private:
	time_t _minOldTime;

};

