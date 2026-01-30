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

#include <chrono>
#include <cstdint>

#include "TimeMgr.h"

TimeMgr::TimeMgr() : _minOldTime(1704063600)
{ }

TimeMgr::~TimeMgr() { }

std::uint64_t TimeMgr::GetUnixTime()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

time_t TimeMgr::GetUnixTimeAsTime()
{
	auto now = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(now);
}

std::uint64_t TimeMgr::ReturnPastOfThirtyDays()
{
	auto now = std::chrono::system_clock::now();

	// Calculate the duration of 30 days in seconds
	auto thirtyDaysAgo = now - std::chrono::hours(24 * 30);

	// Set the time to 00:00:00
	auto startOfDay = std::chrono::time_point_cast<std::chrono::days>(thirtyDaysAgo);

	// Convert the time in seconds since the epoch
	return std::chrono::duration_cast<std::chrono::seconds>(startOfDay.time_since_epoch()).count();
}

std::uint64_t TimeMgr::ReturnPastOfDays(std::uint32_t returnDays)
{
	auto now = std::chrono::system_clock::now();

	auto DaysAgo = now - std::chrono::hours(24 * returnDays);

	// Set the time to 00:00:00
	auto startOfDay = std::chrono::time_point_cast<std::chrono::days>(DaysAgo);

	// Convert the time in seconds since the epoch
	return std::chrono::duration_cast<std::chrono::seconds>(startOfDay.time_since_epoch()).count();
}

std::uint64_t TimeMgr::GetDTETimeAsUint(QDateTimeEdit& timeEdit)
{
	std::uint64_t unixTime = 0;
	if (timeEdit.dateTime().toSecsSinceEpoch() >= 0)
		 unixTime = static_cast<std::uint64_t>(timeEdit.dateTime().toSecsSinceEpoch());

	return unixTime;
}

std::uint64_t TimeMgr::GetDTETimeAsUint(const QDateTimeEdit& timeEdit)
{
	std::uint64_t unixTime = 0;
	if (timeEdit.dateTime().toSecsSinceEpoch() >= 0)
		unixTime = static_cast<std::uint64_t>(timeEdit.dateTime().toSecsSinceEpoch());

	return unixTime;
}

std::string TimeMgr::GetTimeEditTimeAsString(QDateTimeEdit& timeEdit)
{
	std::uint64_t unixTime = 0;
	if (timeEdit.dateTime().toSecsSinceEpoch() >= 0)
		unixTime = static_cast<std::uint64_t>(timeEdit.dateTime().toSecsSinceEpoch());

	return std::to_string(unixTime);
}

std::string TimeMgr::GetTimeEditTimeAsString(const QDateTimeEdit& timeEdit)
{
	std::uint64_t unixTime = 0;
	if (timeEdit.dateTime().toSecsSinceEpoch() >= 0)
		unixTime = static_cast<std::uint64_t>(timeEdit.dateTime().toSecsSinceEpoch());

	return std::to_string(unixTime);
}

std::string TimeMgr::ConvertUnixTimeToString(time_t unixTime)
{
	std::tm timeinfo = {};

#if defined(_WIN32) || defined(_WIN64)
	// Windows platform: Use gmtime_s
	if (gmtime_s(&timeinfo, &unixTime) != 0)
		return "";
#else
	// POSIX platform: Use gmtime_r
	if (gmtime_r(&unixTime, &timeinfo) == nullptr)
		return "";
#endif

	std::ostringstream ss;

	ss << std::put_time(&timeinfo, "%d-%m-%Y %H:%M:%S");

	return ss.str();
}

std::string TimeMgr::ConvertUnixTimeToStringWithDefaultValue(time_t unixTime)
{
	if (_minOldTime <= unixTime)
	{
		std::tm timeinfo = {};

		// Use the safer gmtime_s function on Windows
#if defined(_WIN32) || defined(_WIN64)
		if (gmtime_s(&timeinfo, &unixTime) != 0)
			return "";
#else
	// Use gmtime_r on POSIX platforms (Linux, macOS)
		if (gmtime_r(&unixTime, &timeinfo) == nullptr)
			return "";
#endif

		std::stringstream ss;

		ss << std::put_time(&timeinfo, "%d-%m-%Y %H:%M:%S");

		return ss.str();
	}
	
	return TranslateText::translateNew("TimeString", "No Time Available").toStdString();
}

std::pair<std::string, std::string> TimeMgr::ConvertUnixTimeToDataTimeEdit(time_t unixTime)
{
	std::tm timeinfo = {};

	// Use the safer gmtime_s function on Windows
#if defined(_WIN32) || defined(_WIN64)
	if (gmtime_s(&timeinfo, &unixTime) != 0)
		return std::make_pair("", "");
#else
	// Use gmtime_r on POSIX platforms (Linux, macOS)
	if (gmtime_r(&unixTime, &timeinfo) == nullptr)
		return std::make_pair("", "");
#endif

	std::stringstream ss;

	ss << std::put_time(&timeinfo, "%d-%m-%Y %H:%M:%S");

	return std::make_pair(ss.str(), "%d-%m-%Y %H:%M:%S");
}

QDateTime TimeMgr::ConvertTimeToQDataTime(std::pair<std::string, std::string> timeToConvert)
{
	auto& timeResult1 = timeToConvert;
	std::string formattedDateTime = timeResult1.first;
	std::string setDateTimeFormat = timeResult1.second;

	return QDateTime::fromString(formattedDateTime.c_str(), setDateTimeFormat.c_str());
}

