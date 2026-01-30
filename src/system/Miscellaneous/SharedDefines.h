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

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <string>
#include <map>

#include "DatabaseDefines.h"
#include "Util.h"
#include "TranslateText.h"
#include "Duration.h"

enum class ErrorCodes : std::uint64_t
{
	ERROR_NO_ERRORS									= 0x000000000001,
	ERROR_NO_MACHINE_NAME							= 0x000000000002,
	ERROR_NO_FIRST_NAME								= 0x000000000004,
	ERROR_NO_LAST_NAME								= 0x000000000008,
	ERROR_UNK_ERROR									= 0x000000000010,
	ERROR_FILE_NAME_TO_LONG							= 0x00000000000B,
	ERROR_NO_SAVE_PATH								= 0x00000000000C,
	ERROR_NO_MYSQL_CONNECTION						= 0x00000000000D,
	ERROR_NO_STYLE									= 0x00000000000E,
	ERROR_NO_SETTING_FILE_FOUND						= 0x00000000000F,
	WARNING_PASSWORD_NOT_MATCHED					= 0x000000000016,
	WARNING_PASSWORD_OR_USER_WRONG					= 0x000000000017,
	WARNING_USER_NAME_TO_SHORT						= 0x000000000018,
	WARNING_PASSWORD_TO_SHORT						= 0x000000000019,
	WARNING_USER_ALREADY_EXIST						= 0x00000000001A,
	ERROR_USER_WAS_NOT_CREATED_INTERN				= 0x00000000001B,
	WARNING_OLD_AND_NEW_PASSWORD_SAME				= 0x00000000001C,
	ERROR_PASSWORD_CHANGE_DID_NOT_WORK				= 0x00000000001D,
	ERROR_MYSQL_ERROR								= 0x00000000001E,
	ERROR_NO_LOG_PATH_FOUND							= 0x00000000001F,
	ERROR_LOG_FILE_NOT_OPEN_OR_CREATED				= 0x000000000020,
	ERROR_EXECUTE_SQL_PROBLEM						= 0x000000000021,
	ERROR_SSH_HOSTNAME_ERROR						= 0x000000000022,
	ERROR_SSH_PORT_ERROR							= 0x000000000023,
	ERROR_SSH_USERNAME_ERROR						= 0x000000000024,
	ERROR_SSH_PASSWORD_ERROR						= 0x000000000025,
	ERROR_SSH_PROGRAM_PATH_ERROR					= 0x000000000026,
	ERROR_SSH_IS_ACTIVE_ERROR						= 0x000000000027,
	ERROR_SSH_LOCAL_PORT_ERROR						= 0x000000000028,
	ERROR_NO_USERNAME_IN_FIELD						= 0x000000000029,
	ERROR_NO_PASSWORD_IN_FIELD						= 0x00000000002A,
	WARNING_NO_EMAIL_ADDRESS_ADDED					= 0x00000000002B,
	WARNING_NO_VALID_EMAIL_ADDRESS					= 0x00000000002C,
	WARNING_EMAIL_ADDRESS_ALREADY_USED				= 0x00000000002D,
	WARNING_SOUND_GENERAL_WARNING					= 0x00000000002E,
	WARNING_CURRENT_LANGUAGE						= 0x00000000002F,
	WARNING_USERNAME_ALREADY_IN_USE					= 0x000000000030,
	WARNING_EMAIL_ADDRESS_IN_USE					= 0x000000000031,
	WARNING_EMAIL_NOT_CHANGED						= 0x000000000032,
	WARNING_ACCESS_RIGHT_NOT_CHANGED				= 0x000000000033,
	WARNING_NO_CHIP_ID								= 0x000000000034,
	WARNING_BOX_ALREADY_EXIST						= 0x000000000035,
	WARNING_BOX_NAME_UPDATE_FAILED					= 0x000000000036,
	WARNING_ARTICLE_POSITION_ALREADY_EXIST			= 0x000000000037,
	WARNING_ARTICLE_POSITION_INSERT_ERROR			= 0x000000000038,
	WARNING_ARTICLE_POSITION_BOX_UPDATE				= 0x000000000039,
	WARNING_ARTICLE_OUTSOURCE_ERROR_GENERAL			= 0x00000000003A,
	WARNING_ARTICLE_OUTSOURCE_TO_LOW_COUNT			= 0x00000000003B,
	WARNING_ARTICLE_OUTSOURCE_NO_DATA				= 0x00000000003C,
	WARNING_ARTICLE_EDIT_NO_ROW_SELECTED			= 0x00000000003D,
	WARNING_ARTICLE_EDIT_MULTIPLE_ROWS				= 0x00000000003E,
	WARNING_ARTICLE_INSERT_ARTICLE_EXIST			= 0x00000000003F,
	WARNING_ARTICLE_INSERT_MISSING_NAME				= 0x000000000040,
	WARNING_ARTICLE_INSERT_MISSING_NUMBER			= 0x000000000041,
	WARNING_ARTICLE_INSERT_MISSING_MANUFACTURER		= 0x000000000042,
	WARNING_ARTICLE_POSITION_NOT_SET				= 0x000000000043,
	WARNING_ARTICLE_POSITION_NOT_UPDATED			= 0x000000000044,
	WARNING_ARTICLE_BOX_NOT_EXIST					= 0x000000000045,
	WARNING_ARTICLE_NOT_DELETED						= 0x000000000046,
	WARNING_ARTICLE_BOX_POSITION_IN_USE				= 0x000000000047,
	WARNING_ARTICLE_BOX_NAME_EXIST_ADD_NEW			= 0x000000000048,
	WARNING_ARTICLE_MISSING_POSITION_AISLE			= 0x000000000049,
	WARNING_ARTICLE_MISSING_POSITION_SIDE			= 0x00000000004A,
	WARNING_ARTICLE_MISSING_POSITION_COLUMN			= 0x00000000004B,
	WARNING_ARTICLE_MISSING_POSITION_ROW			= 0x00000000004C,
	WARNING_ARTICLE_MISSING_BOX_NAME				= 0x00000000004D,
	WARNING_ARTICLE_BOX_NAME_TO_SHORT				= 0x00000000004E,
	WARNING_ARTICLE_BOX_UPDATE_FAILED				= 0x00000000004F,
	WARNING_ARTICLE_BOX_ADD_FAILED					= 0x000000000050,
	WARNING_ARTICLE_BOX_REALY_DELETE				= 0x000000000051,
	WARNING_USER_OLD_PASSWORD_REQUIRED				= 0x000000000052,
	ERROR_USER_CAN_NOT_CHANGE_PASSWORD_OTHER_USER	= 0x000000000053,
	WARNING_USER_PASSWORD_NOT_FOUND					= 0x000000000054,
	WARNING_USER_OLD_PASSWORD_WRONG					= 0x000000000055,
	ERROR_USER_CAN_NOT_CHANGE_OTHER_USER_MAIL		= 0x000000000056,
	ERROR_USER_CAN_NOT_CHANGE_OTHER_USER_RFID		= 0x000000000057,
	WARNING_RFID_DATA_IN_USE						= 0x000000000058,
	WARNING_RFID_CAN_NOT_BE_CHANGED					= 0x000000000059,
	ERROR_USER_CAN_NOT_CHANGE_OTHER_USER_RIGHTS		= 0x00000000005A,
	WARNING_ARTICLE_BARCODE_ALREADY_IN_USE			= 0x00000000005B,
	WARNING_ARTICLE_BOX_ARTICLE_NO_UPDATED			= 0x00000000005C,
	WARNING_SETTINGS_FULL_SCREEN					= 0x00000000005D,
	WARNING_ADMIN_CHANGE_USER_MAIL					= 0x00000000005E,
	WARNING_ADMIN_CHANGE_USER_PASSWORD				= 0x00000000005F,
	WARNING_ADMIN_CHANGE_RFID						= 0x000000000060,
	WARNING_PASSWORD_EMPTY							= 0x000000000061,
	WARNING_COST_UNIT_NEED_TO_SELECTED				= 0x000000000062,
	WARNING_COST_UNIT_CORRECT_SELECTED				= 0x000000000063,
	ERROR_NOT_ENOUGH_RIGHTS							= 0x000000000064,
	WARNING_ARTICLE_STORE_NEED_QUANTITY				= 0x000000000065,
	WARNING_MESSAGE_PARTIAL_DELIVERY				= 0x000000000066,
	WARNING_DELETE_BOX_MUST_UPDATE_POSITION			= 0x000000000067,
	WARNING_ARTICLE_STORE_MORE_AS_ORDERED			= 0x000000000068,
	WARNING_DELETE_BARCODE_CONFIRM					= 0x000000000069,
	WARNING_PASSWORD_HAS_NO_UPPER_CASE				= 0x00000000006A,
	WARNING_PASSWORD_HAS_NO_LOWER_CASE				= 0x00000000006B,
	WARNING_PASSWORD_HAS_NO_DIGIT					= 0x00000000006C,
	WARNING_PASSWORD_HAS_NO_SPECIAL_CHAR			= 0x00000000006D,
	WARNING_CHANGE_OWN_PASSWORD_NEED_OLD_PASSWORD	= 0x00000000006E,
	WARNING_YOU_CAN_NOT_DELETE_YOURSELF				= 0x00000000006F,
	WARNING_NO_IMAGE_TO_PRINT						= 0x000000000070,
	ERROR_NO_SHOW_ERROR_OR_WARNING					= 0x000000000071,
	WARNING_BACK_WITHOUT_SAVE						= 0x000000000072,
	ERROR_NOT_ENOUGH_ARTICLE_COUNT					= 0x000000000073,
	WARNING_SHOPPING_CART_DELETED					= 0x000000000074,
	WARNING_NOT_ENOUGH_ARTICLE_IN_STOCK				= 0x000000000075,
	WARNING_IMAGE_CAN_NOT_SET_FOR_ARTICLE			= 0x000000000076,
	WARNING_IMAGE_PATH_NOT_VALID_OR_NO_IMAGE		= 0x000000000077,
	WARNING_USER_DELETE_SOMETHING_BE_WRONG			= 0x000000000078,
	WARNING_UNSAVED_PERMISSION_CHANGES				= 0x000000000079,
	WARNING_UNSAVED_USER_CHANGES					= 0x00000000007A,
	WARNING_NO_ACCESS_RIGHTS_SELECTED				= 0x00000000007B,
	WARNING_NO_COMPANY_LOCATION_SET					= 0x00000000007C,
	WARNING_NO_STORAGE_ROOM_SET						= 0x00000000007D,
	WARNING_IMAGE_PATH_INVALID						= 0x00000000007E,
	WARNING_IMAGE_NOT_A_VALID_FORMAT				= 0x00000000007F,
	WARNING_ARTICLE_ALREADY_IN_TABLE				= 0x000000000080,
	WARNING_ARTICLE_COUNT_CAN_NOT_BE_0				= 0x000000000081,
	WARNING_ARTICLE_COUNT_TO_HIGH					= 0x000000000082,
	WARNING_PFD_ONLY_6_ENTRY						= 0x000000000083,
	WARNING_NO_ORDERS_IN_TABLE						= 0x000000000084,
	WARNING_NOT_LOGGED_IN_ODER_PLACE				= 0x000000000085,
	WARNING_NO_COST_UNIT_SELECTED					= 0x000000000086,
	WARNING_STORAGE_POSITION_DUPLICATE				= 0x000000000087,
	WARNING_FILE_USED_FOR_OTHER_ORDER				= 0x000000000088,
	WARNING_FILE_UPDATE_ADD_ERROR					= 0x000000000089,
	// Return Shipment
	WARNING_RETURN_NO_INTERN_CONTACT_PERSON			= 0x00000000008A,	// 138
	WARNING_RETURN_NO_COST_UNIT						= 0x00000000008B,	// 139
	WARNING_RETURN_NO_RECEIVER						= 0x00000000008C,	// 140
	WARNING_RETURN_NO_STATUS						= 0x00000000008D,	// 141
	// Room Manager
	WARNING_ROOM_EXIST_WITH_SAME_NAME				= 0x00000000008E,	// 142
	WARNING_ROOM_CREATE_FAILED						= 0x00000000008F,	// 143
	WARNING_ROOM_UPDATE_FAILED						= 0x000000000090,	// 144
	WARNING_ROOM_NOT_EMPTY							= 0x000000000091,	// 145

	// AMS Warnings
    WARNING_AMS_MISSING_REQUIRED_FIELDS				= 0x0000000000A0,	// 160
    WARNING_AMS_CALLER_DELETE_NOT_WORKING			= 0x0000000000A1,	// 161
    WARNING_AMS_CALLER_NOT_ADDED					= 0x0000000000A2,	// 162
    WARNING_AMS_SURE_DELETE_CALLER					= 0x0000000000A3,	// 163
    WARNING_AMS_EMPLOYEE_ADD_NOT_WORKING			= 0x0000000000A4,   // 164
    WARNING_AMS_EMPLOYEE_UPDATE_NOT_WORKING			= 0x0000000000A5,	// 165
    WARNING_AMS_SURE_DELETE_EMPLOYEE				= 0x0000000000A6,	// 166
    WARNING_AMS_EMPLOYEE_DELETE_NOT_WORKING			= 0x0000000000A7,	// 167
};

enum class SuccessfullCodes : std::uint64_t
{
	MESSAGE_CODE_NO_DATA								= 0xDEAD000,
	MESSAGE_USER_SUCCESSFULLY_CREATED					= 0xDEAD001,
	MESSAGE_ACCESS_RIGHTS_SUCCESSFULLY_CHANGED			= 0xDEAD002,
	MESSAGE_PASSWORD_SUCCESSFULLY_CHANGED				= 0xDEAD003,
	MESSAGE_EMAIL_SUCCESSFULLY_CHANGED					= 0xDEAD004,
	MESSAGE_ARTICLE_SUCCESSFULLY_OUTSOURCE				= 0xDEAD005,
	MESSAGE_ARTICLE_SUCCESSFULLY_STORED					= 0xDEAD006,
	MESSAGE_ARTICLE_POSITION_IN_BOX_SUCCESSFULLY_UPDATE	= 0xDEAD007,
	MESSAGE_ARTICLE_POSITION_UPDATED					= 0xDEAD008,
	MESSAGE_ARTICLE_SUCCESSFULLY_UPDATED				= 0xDEAD009,
	MESSAGE_ARTICLE_SUCCESSFULLY_DELETED				= 0xDEAD010,
	MESSAGE_ARTICLE_BOX_SUCCESSFULLY_UPDATED			= 0xDEAD011,
	MESSAGE_ARTICLE_BOX_SUCCESSFULLY_ADDED				= 0xDEAD012,
	MESSAGE_ARTICLE_BOX_DELETED							= 0xDEAD013,
	MESSAGE_USER_SUCCESSFULLY_CHANGED					= 0xDEAD014,
	MESSAGE_USER_RFID_SUCCESSFULLY_CHANGED				= 0xDEAD015,
	MESSAGE_ERROR_NO_DATA								= 0xDEAD016,
	MESSAGE_SETTINGS_SUCCESSFULLY_SAVED					= 0xDEAD017,
	MESSAGE_ARTICLE_SUCCESSFULLY_ADDED					= 0xDEAD018,
	MESSAGE_ORDER_SUCCESSFULLY_AND_COMPLETE_STORED		= 0xDEAD019,
	MESSAGE_ORDER_SUCCESSFULLY_PARTIAL_STORED			= 0xDEAD020,
	MESSAGE_SUCCESSFULLY_ADD_TO_SHOPPING_CART			= 0xDEAD021,
	MESSAGE_SUCCESSFULLY_SHOPPING_CART_OUTSOURCED		= 0xDEAD022, // With arguments
	MESSAGE_SUCCESSFULLY_USER_DELETED					= 0xDEAD023,
	// Room Manager
	MESSAGE_SUCCESSFULLY_ROOM_ADDED						= 0xDEAD024,
	MESSAGE_SUCCESSFULLY_ROOM_UPDATED					= 0xDEAD025,
	MESSAGE_SUCCESSFULLY_ROOM_DELETED					= 0xDEAD026,

	// AMS Messages
	MESSAGE_AMS_CALLER_SUCCESSFULLY_ADDED				= 0xDEAD030,  // 233492528
	MESSAGE_AMS_CALLER_SUCCESSFULLY_DELETED				= 0xDEAD031,  // 233492529
    MESSAGE_AMS_CALLER_SUCCESSFULLY_UPDATED				= 0xDEAD032,  // 233492530
    MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_ADDED				= 0xDEAD033,  // 233492531
    MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_UPDATED			= 0xDEAD034,  // 233492532
    MESSAGE_AMS_EMPLOYEE_SUCCESSFULLY_DELETED			= 0xDEAD035,  // 233492533
};

enum OrderNumber : std::uint8_t
{
	ORDER_1									= 1,
	ORDER_2									= 2,
	ORDER_3									= 3,
	ORDER_4									= 4,
	ORDER_5									= 5,
	ORDER_6									= 6,

	MAX_ORDER
};

enum class Styles
{
	STYLE_LIGHT								= 0,
	STYLE_DARK								= 1,
};

enum class OrderStatus : std::uint8_t
{
	ORDER_STATUS_UNORDERED					= 1,
	ORDER_STATUS_WAITING_FOR_OFFER			= 2,
	ORDER_STATUS_ORDERED					= 3,
	ORDER_STATUS_DELIVERED					= 4,
	ORDER_STATUS_NOT_AVAILABLE				= 5,
	ORDER_STATUS_DELIVERY_DELAY				= 6,
	ORDER_STATUS_INCORRECT					= 7,
	ORDER_STATUS_STORED						= 8, // only possible if status >= ORDER_STATUS_ORDERED | Stored is == Delivered
	ORDER_STATUS_ORDER_CONFIRMATION			= 9, // 8
	ORDER_STATUS_OFFER_RECEIVED				= 10, // 9
	ORDER_STATUS_PARTIAL_DELIVERED			= 11, // 10
};

enum class OrderFinalStatus
{
	ORDER_FINAL_STATUS_IN_PROGRESS			= 0,
	ORDER_FINAL_STATUS_DELIVERED			= 1, // include stored
	ORDER_FINAL_STATUS_INCORRECT			= 2, // This status if it is an incorrect order.
};

enum class ArticleDatabaseState : std::uint8_t
{
	ARTICLE_STATE_IS_ACTIVE					= 0,
	ARTICLE_STATE_IS_DELETED				= 1,
};

enum class AccessRights : std::uint8_t
{
	ACCESS_RIGHT_NONE						= 0,	// Logout							-> Nichts
	ACCESS_RIGHT_NORMAL						= 1,	// All normal workman				-> Artikeln ansehen und auslagern.
	ACCESS_RIGHT_NORMAL_PLUS				= 2,	// All normal workman				-> Artikel einlagern.
	ACCESS_RIGHT_WAREHOUSEMAN				= 3,	// Lagermitarbeiter					-> Artikel anlegen und aendern, Box Anlegen und aendern
	ACCESS_RIGHT_USERMANAGER				= 4,	// Usermanager / Werkstattleitung	-> User anlegen und Rechte verwalten (Bis einschliesslich stufe 5)
	ACCESS_RIGHT_ADMIN						= 5,	// Admin							-> Datenbank einstellungen aendern 
};

enum class ReturnStatus : std::uint8_t
{
	RETURN_STATUS_NONE						= 0,
	RETURN_STATUS_IN_PREPARATION			= 1,
	RETURN_STATUS_SHIPPING					= 2,
	RETURN_STATUS_RECEIVED_BACK				= 3,
	RETURN_STATUS_CREDIT_RECEIVED			= 4,
};

enum class GrindingStatus : std::uint8_t
{
	GRINDING_STATUS_NONE					= 0,
	GRINDING_STATUS_TO_GRINDING				= 1,
	GRINDING_STATUS_FROM_GRINDING_BACK		= 2,
	GRINDING_STATUS_UNGRINDABLE				= 3,
	GRINDING_STATUS_DELETED					= 4,
};

enum class OldDataData : std::uint8_t
{
	OLD_DATA_ARTICLE						= 0,
	OLD_DATA_CONTACT						= 1,
	OLD_DATA_ORDER							= 2,
	OLD_DATA_AUXILIARY						= 4,
};

enum class SelectLanguage : std::uint8_t
{
	LANG_EN_GB								= 0,
	LANG_DE_DE								= 1,
	LANG_JP_JP								= 2,
	LANG_PL_PL								= 3,
	LANG_RU_RU								= 4,

	MAX_LANGUAGE,
};

inline const std::string stringLang_en_GB = "en_GB";
inline const std::string stringLang_de_DE = "de_DE";
inline const std::string stringLang_ja_JP = "jp_JP";
inline const std::string stringLang_pl_PL = "pl_PL";
inline const std::string stringLang_ru_RU = "ru_RU";


inline std::string GetSelectLanguageString(SelectLanguage _language)
{
	switch (_language)
	{
		case SelectLanguage::LANG_EN_GB: return "en_GB";
		case SelectLanguage::LANG_DE_DE: return "de_DE";
		case SelectLanguage::LANG_JP_JP: return "jp_JP";
		case SelectLanguage::LANG_PL_PL: return "pl_PL";
		case SelectLanguage::LANG_RU_RU: return "ru_RU";
		default: return "en_GB";
	}
}

inline std::uint8_t GetSelectLanguageID(const std::string& _language)
{
	static const std::unordered_map<std::string, SelectLanguage> languageMap = {
			{stringLang_en_GB, SelectLanguage::LANG_EN_GB},
			{stringLang_de_DE, SelectLanguage::LANG_DE_DE},
			{stringLang_ja_JP, SelectLanguage::LANG_JP_JP},
			{stringLang_pl_PL, SelectLanguage::LANG_PL_PL},
			{stringLang_ru_RU, SelectLanguage::LANG_RU_RU},
	};

	auto it = languageMap.find(_language);
	if (it != languageMap.end())
		return static_cast<std::uint8_t>(it->second);

	// Default to English if not found
	return static_cast<std::uint8_t>(SelectLanguage::LANG_EN_GB);
}

enum class LowAlert : std::uint8_t
{
	LOW_ALERT_NONE					= 0,
	LOW_ALERT_ORDER_CREATE			= 1,
	LOW_ALERT_ORDERED				= 2,
	LOW_ALERT_DELIVERED				= 3,
	LOW_ALERT_STORED				= 4,
	LOW_ALERT_CANCELED_ORDER		= 5,
};

enum class ArticleUnit : std::uint8_t
{
	ARTICLE_UNIT_PIECE				= 0,
	ARTICLE_UNIT_METER				= 1,
	ARTICLE_UNIT_LITER				= 2,
};

inline std::string GetArticleUnitString(ArticleUnit unit)
{
	switch (unit)
	{
		case ArticleUnit::ARTICLE_UNIT_PIECE: return TranslateText::translateNew("ArticleUnit", "Piece").toStdString();
		case ArticleUnit::ARTICLE_UNIT_METER: return TranslateText::translateNew("ArticleUnit", "Meter").toStdString();
		case ArticleUnit::ARTICLE_UNIT_LITER: return TranslateText::translateNew("ArticleUnit", "Liter").toStdString();
		default: return TranslateText::translateNew("ArticleUnit", "Unknown Unit").toStdString();
	}
}

enum class DialogButtonPressed : std::uint8_t
{
	BUTTON_PRESSED_NONE				= 0,
	BUTTON1_PRESSED					= 1,
	BUTTON2_PRESSED					= 2,
};

enum class CompanyLocations : int
{
	CL_NONE						= 0x000,	// None
	CL_BBG						= 0x001,	// Bueckeburg
	CL_KNR						= 0x002,	// Koennern
	CL_WISL						= 0x004,	// Wiefelstede
	CL_HGN						= 0x008,	// Hagenah Frische GmbH
	CL_FRN						= 0x010,	// Freienbrink
    CL_BBG_NLZ                  = 0x020,    // Bueckeburg NLZ
};

inline std::string GetCompanyLocationsString(CompanyLocations cl)
{
	switch (cl)
	{
		case CompanyLocations::CL_BBG: return "BBG";
		case CompanyLocations::CL_KNR: return "KNR";
		case CompanyLocations::CL_WISL: return "WISL";
		case CompanyLocations::CL_HGN: return "HGN";
	    case CompanyLocations::CL_FRN: return "FRN";
	    case CompanyLocations::CL_BBG_NLZ: return "BBG_NLZ";
		default: return "BBG";
	}
}

struct ArticleBoxData
{
	std::uint32_t boxID = 0;
	std::string boxName{};
	std::string BoxAisle{};
	std::string BoxSide{};
	std::string BoxColumn{};
	std::string BoxRow{};
	CompanyLocations cLocation;
	std::uint32_t roomLocation = 0;
	std::uint64_t positionGUID = 0;

	ArticleBoxData() : cLocation(CompanyLocations::CL_BBG) {}
};

inline std::string GetOrderStatusString(OrderStatus status)
{
	switch (status)
	{
		case OrderStatus::ORDER_STATUS_UNORDERED: return TranslateText::translateNew("OrderStatusString", "Unordered").toStdString();
		case OrderStatus::ORDER_STATUS_WAITING_FOR_OFFER: return TranslateText::translateNew("OrderStatusString", "Waiting for Offer").toStdString();
		case OrderStatus::ORDER_STATUS_ORDERED: return TranslateText::translateNew("OrderStatusString", "Ordered").toStdString();
		case OrderStatus::ORDER_STATUS_DELIVERED: return TranslateText::translateNew("OrderStatusString", "Delivered").toStdString();
		case OrderStatus::ORDER_STATUS_NOT_AVAILABLE: return TranslateText::translateNew("OrderStatusString", "Not Available").toStdString();
		case OrderStatus::ORDER_STATUS_DELIVERY_DELAY: return TranslateText::translateNew("OrderStatusString", "Delivery Delay").toStdString();
		case OrderStatus::ORDER_STATUS_INCORRECT: return TranslateText::translateNew("OrderStatusString", "Incorrect Order").toStdString();
		case OrderStatus::ORDER_STATUS_STORED: return TranslateText::translateNew("OrderStatusString", "Stored").toStdString();
		default: return TranslateText::translateNew("OrderStatusString", "No Order Status").toStdString();
	}
}

enum class PartialDeliverStatus : std::uint8_t
{
	PARTIAL_DELIVER_STATUS_INCOMPLETE			= 0,
	PARTIAL_DELIVER_STATUS_COMPLETE				= 1,
};

enum class LogFilterFlags : std::uint32_t
{
	LOG_FLAG_CREATE_ARTICLE					= 0x00001,
	LOG_FLAG_CHANGE_ARTICLE					= 0x00002,
	LOG_FLAG_DELETE_ARTICLE					= 0x00004,
	LOG_FLAG_CHANGE_ARTICLE_COUNT			= 0x00008,
	LOG_FLAG_CREATE_ARTICLE_BOX				= 0x00010,
	LOG_FLAG_CHANGE_ARTICLE_BOX				= 0x00020,
	LOG_FLAG_DELETE_ARTICLE_BOX				= 0x00040,
	LOG_FLAG_CREATE_USER					= 0x00080,
	LOG_FLAG_CHANGE_USER					= 0x00100,
	LOG_FLAG_DELETE_USER					= 0x00200,
	LOG_FLAG_CHANGE_SETTINGS				= 0x00400,
};

enum class OrderPriority : std::uint8_t
{
	ORDER_PRIORITY_NORMAL	= 0,
	ORDER_PRIORITY_EXPRESS	= 1,
	ORDER_PRIORITY_COURIER	= 2,
};

enum class LocalCommunicationCommands : std::uint8_t
{
	COMMAND_OPEN_BARCODEWINDOW_FROM_IMS		= 1,
};

struct SerialSettings
{
	std::string portName;
	QSerialPort::BaudRate baudRate;
	QSerialPort::DataBits dataBits;
	QSerialPort::Parity parity;
	QSerialPort::StopBits stopBits;
	QSerialPort::FlowControl flowControl;
};

enum class maxColumnWidth : std::uint8_t
{
	STANDART = 150,
};

enum class WindowNames : std::uint8_t
{
	WINDOW_NONE					= 0,
	WINDOW_MAIN_WINDOW			= 1,
	WINDOW_ARTICLE_MANAGER		= 2,
	WINDOW_HISTORY				= 3,
	WINDOW_ORDER_MANAGER		= 4,
	WINDOW_BOX_MANAGER			= 5,
};

enum class BarcodeType : std::uint8_t
{
	BARCODE_ARTICLE				= 0,	// XX-00000000000000000000 (XX = RDN & 000 = RDN)
	BARCODE_RETURN_SLIP			= 1,	// BRS-0000000000000000000 (BRS = Fix & Fill "0" Last Digits is the Return ID)
	BARCODE_COST_UNIT			= 2,	// BCU-0000000000000000000 (BCU = Fix & 000 = RDN)
	BARCODE_MO					= 3,	// BMO-%COSTUNIT%-%LINENUMBER%-%UNIXTIME%	(Maintenance Order)
	BARCODE_CONTROL_TTS         = 4,	// TTS-BG000	(000 = RDN)
};

inline std::string GetWindowNameString(WindowNames name)
{
	switch (name)
	{
		case WindowNames::WINDOW_MAIN_WINDOW: return "mainWindow";
		case WindowNames::WINDOW_ARTICLE_MANAGER: return "articleManagerWindow";
		case WindowNames::WINDOW_HISTORY: return "historyWindow";
		case WindowNames::WINDOW_ORDER_MANAGER: return "orderManagerWindow";
		case WindowNames::WINDOW_BOX_MANAGER: return "boxManagerWindow";
		default: return "NoWindow";
	}
}

inline WindowNames GetWindowsEnum(std::string name)
{
	if (name == "mainWindow")
		return WindowNames::WINDOW_MAIN_WINDOW;
	else if (name == "articleManagerWindow")
		return WindowNames::WINDOW_ARTICLE_MANAGER;
	else if (name == "historyWindow")
		return WindowNames::WINDOW_HISTORY;
	else if (name == "orderManagerWindow")
		return WindowNames::WINDOW_ORDER_MANAGER;
	else if (name == "boxManagerWindow")
		return WindowNames::WINDOW_BOX_MANAGER;

	return WindowNames::WINDOW_NONE;
}

struct WindowGeometry
{
	QSize size;
	QPoint position;
};

struct ImageData
{
	std::vector<char> imageBuffer{};
	std::string fileExtension{};
};

struct CostUnitTranslations
{
	std::uint32_t internalID{};
	std::uint32_t costUnitID{};
	std::string costUnitName{};
	std::string place{};
	std::string language{};

	CostUnitTranslations() :
		internalID(0), costUnitID(0), costUnitName(""), place(""), language("") {
	}

	CostUnitTranslations(std::uint32_t iID, std::uint32_t costID, std::string name, std::string plc, std::string lang) :
		internalID(iID), costUnitID(costID), costUnitName(name), place(plc), language(lang) {}
};

struct StorageRoom
{
	int id;              // Unique ID of the storage room
	std::string name;    // Name of the storage room
	CompanyLocations location = CompanyLocations::CL_NONE; // Location of the storage room

	StorageRoom(int id = 0, std::string name = "") :
		id(id), name(name) {}


	// Optional: Comparison operator for sorting based on the ID
	bool operator<(const StorageRoom& other) const
	{
		return id < other.id;
	}

	bool operator>(const StorageRoom& other) const
	{
		return id > other.id;
	}
};

struct CompanyLocationData
{
	int id;
	std::string locAbbr;
	std::string FullName;

	CompanyLocationData() :
		id(0), locAbbr(""), FullName("") {}

	bool operator<(const CompanyLocationData& other) const
	{
		return id < other.id;
	}

	bool operator>(const CompanyLocationData& other) const
	{
		return id > other.id;
	}
};

struct ArticlePositionData
{
	std::uint64_t positionID = 0;
	std::uint32_t articleID = 0;
	std::string aisle{};
	std::string side{};
	std::string tier{};
	std::string shelfRow{};
	std::uint32_t boxID = 0;
	std::string boxName{};
	CompanyLocations cLoaction = CompanyLocations::CL_NONE;
	std::uint32_t roomIndex = 0;

	// Default ctor for containers
	ArticlePositionData() = default;

	// Canonical ctor (stores std::string)
	ArticlePositionData(std::uint64_t positionGUID,
		std::uint32_t articleID_,
		std::string aisle_,
		std::string side_,
		std::string tier_,
		std::string shelfRow_,
		std::uint32_t boxID_,
		std::string boxName_,
		CompanyLocations cLocation_,
		std::uint32_t roomIndex_)
		: positionID(positionGUID),
		articleID(articleID_),
		aisle(std::move(aisle_)),
		side(std::move(side_)),
		tier(std::move(tier_)),
		shelfRow(std::move(shelfRow_)),
		boxID(boxID_),
		boxName(std::move(boxName_)),
		cLoaction(cLocation_),
		roomIndex(roomIndex_)
	{
		// no-op
	}

	// Forwarding ctor: accepts sql::SQLString, std::string_view, const char*, ...
	template<typename S1, typename S2, typename S3, typename S4, typename S5>
	ArticlePositionData(std::uint64_t positionGUID,
		std::uint32_t articleID_,
		const S1& aisleAny,
		const S2& sideAny,
		const S3& tierAny,
		const S4& shelfRowAny,
		std::uint32_t boxID_,
		const S5& boxNameAny,
		CompanyLocations cLocation_,
		std::uint32_t roomIndex_)
		: ArticlePositionData(positionGUID,
			articleID_,
			ToStdString(aisleAny),
			ToStdString(sideAny),
			ToStdString(tierAny),
			ToStdString(shelfRowAny),
			boxID_,
			ToStdString(boxNameAny),
			cLocation_,
			roomIndex_)
	{
		// convert generic to std::string once
	}
};

struct ArticleListData
{
	std::uint32_t articleID = 0;
	std::uint16_t currentCount = 0;
	std::uint16_t minimumCount = 0;
	std::uint16_t orderCount = 0;
	std::uint32_t allCompanyCount = 0;
	CompanyLocations cLocation = CompanyLocations::CL_NONE;
	std::uint32_t roomIndex = 0;

	ArticleListData(const std::uint32_t articleID = 0, const std::uint16_t currCount = 0, const std::uint16_t minCount = 0, const std::uint16_t orderCount = 0, std::uint32_t allCount = 0,
		const CompanyLocations& cLoc = CompanyLocations::CL_NONE, std::uint32_t roomIndex = 0)
		: articleID(articleID), currentCount(currCount), minimumCount(minCount), orderCount(orderCount), allCompanyCount(allCount), cLocation(cLoc), roomIndex(roomIndex) {}

	// Comparison operations for minimumCount
	bool minLess(const ArticleListData& other) const
	{
		return minimumCount < other.minimumCount;
	}

	bool minGreater(const ArticleListData& other) const
	{
		return minimumCount > other.minimumCount;
	}

	bool minEqual(const ArticleListData& other) const
	{
		return minimumCount == other.minimumCount;
	}

	// Comparison operations for orderCount
	bool ordLess(const ArticleListData& other) const
	{
		return orderCount < other.orderCount;
	}

	bool ordGreater(const ArticleListData& other) const
	{
		return orderCount > other.orderCount;
	}

	bool ordEqual(const ArticleListData& other) const
	{
		return orderCount == other.orderCount;
	}
};

struct RoomLocationData
{
	std::uint32_t roomID = 0;
	std::string roomName{};
	CompanyLocations cLocation = CompanyLocations::CL_NONE;
};

enum class InternalErrorCodes : std::uint16_t
{
	ERROR_NO_ERROR							= 0x0000,
	ERROR_ROOM_WITH_SAME_NAME_EXIST			= 0x0001,
	ERROR_ROOM_CREATE_FAILED				= 0x0002,
	ERROR_ROOM_UPDATE_FAILED				= 0x0003,
};

struct ArticleDatabaseData
{
	std::uint32_t articleID = 0;
	std::string manufacturer{};
	std::string articleNumber{};
	std::string articleName{};
	std::string replacementNumber{};
	std::string usedInSystem{};
	std::string articleCosts{}; // double articleCosts;
	std::string suppliedBy{};
	std::string moreInformation{};
	std::string knifeAndDiskData{};
	std::string structure{};
	ArticleUnit unit = ArticleUnit::ARTICLE_UNIT_PIECE;
	ArticleDatabaseState state = ArticleDatabaseState::ARTICLE_STATE_IS_ACTIVE;
};

enum class TextToSpeechMode : std::uint8_t
{
	NONE									= 0,
	ARTICLEPOSITION_BOXNAME_AND_COUNT		= 1,
	ARTICLENAME								= 2,
	ARTICLENUMBER							= 3,
	ARTICLENUMBER_AND_ARTICLENAME			= 4,

};

/* ---------------- AMS ------------------------------ */

enum class TicketStatus : std::uint8_t
{
	TICKET_STATUS_NONE								= 0,
	TICKET_STATUS_NEW								= 1,
	TICKET_STATUS_IN_PROGRESS						= 2,
	TICKET_STATUS_SERVICE_SCHEDULED					= 3,
	TICKET_STATUS_WAITING_FOR_PARTS					= 4,
    TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE	= 5,
    TICKET_STATUS_RESOLVED							= 6,
	TICKET_STATUS_CLOSED							= 7,
};

enum class TicketPriority : std::uint8_t
{
    TICKET_PRIORITY_NONE		= 0,
	TICKET_PRIORITY_CRITICAL	= 1,
	TICKET_PRIORITY_HIGH		= 2,
	TICKET_PRIORITY_MEDIUM		= 3,
	TICKET_PRIORITY_LOW			= 4,
    TICKET_PRIORITY_TRIVIAL		= 5,
};

inline QString GetTicketPriorityQString(const TicketPriority p)
{
    switch (p)
	{
		case TicketPriority::TICKET_PRIORITY_CRITICAL: return TranslateText::translateNew("TicketPriority", "Critical");
		case TicketPriority::TICKET_PRIORITY_HIGH: return TranslateText::translateNew("TicketPriority", "High");
		case TicketPriority::TICKET_PRIORITY_MEDIUM: return TranslateText::translateNew("TicketPriority", "Medium");
		case TicketPriority::TICKET_PRIORITY_LOW: return TranslateText::translateNew("TicketPriority", "Low");
		case TicketPriority::TICKET_PRIORITY_TRIVIAL: return TranslateText::translateNew("TicketPriority", "Trivial");
		default: return TranslateText::translateNew("TicketPriority", "No Priority");
    }
}

inline std::string GetTicketPriorityString(const TicketPriority p)
{
    return GetTicketPriorityQString(p).toStdString();
}

inline QString GetTicketStatusQString(const TicketStatus s)
{
	switch (s)
	{
		case TicketStatus::TICKET_STATUS_NEW: return TranslateText::translateNew("TicketStatus", "New");
		case TicketStatus::TICKET_STATUS_IN_PROGRESS: return TranslateText::translateNew("TicketStatus", "In Progress");
		case TicketStatus::TICKET_STATUS_SERVICE_SCHEDULED: return TranslateText::translateNew("TicketStatus", "Service Scheduled");
		case TicketStatus::TICKET_STATUS_WAITING_FOR_PARTS: return TranslateText::translateNew("TicketStatus", "Waiting for Parts");
		case TicketStatus::TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE: return TranslateText::translateNew("TicketStatus", "Waiting for Production Response");
		case TicketStatus::TICKET_STATUS_RESOLVED: return TranslateText::translateNew("TicketStatus", "Resolved");
		case TicketStatus::TICKET_STATUS_CLOSED: return TranslateText::translateNew("TicketStatus", "Closed");
		default: return TranslateText::translateNew("TicketStatus", "No Status");
	}
}

inline std::string GetTicketStatusString(const TicketStatus s)
{
	return GetTicketStatusQString(s).toStdString();
}
	
enum class StatusMessageQueue : std::uint8_t
{
    QUEUE   = 0,
    INSTANT = 1,
};

inline QColor GetTicketStatusColor(TicketStatus s)
{
    switch (s)
    {
        case TicketStatus::TICKET_STATUS_NEW:
            return QColor(52, 152, 219);  // blue
        case TicketStatus::TICKET_STATUS_IN_PROGRESS:
            return QColor(241, 196, 15);  // yellow
        case TicketStatus::TICKET_STATUS_SERVICE_SCHEDULED:
            return QColor(155, 89, 182);  // purple
        case TicketStatus::TICKET_STATUS_WAITING_FOR_PARTS:
            return QColor(230, 126, 34);  // orange
        case TicketStatus::TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE:
            return QColor(211, 84, 0);  // dark orange
        case TicketStatus::TICKET_STATUS_RESOLVED:
            return QColor(46, 204, 113);  // green
        case TicketStatus::TICKET_STATUS_CLOSED:
            return QColor(127, 140, 141);  // gray
        case TicketStatus::TICKET_STATUS_NONE:
        default:
            return QColor(149, 165, 166);  // light gray
    }
}

inline QColor GetTicketPriorityColor(TicketPriority p)
{
    switch (p)
    {
        case TicketPriority::TICKET_PRIORITY_CRITICAL:
            return QColor(192, 57, 43);  // red
        case TicketPriority::TICKET_PRIORITY_HIGH:
            return QColor(230, 126, 34);  // orange
        case TicketPriority::TICKET_PRIORITY_MEDIUM:
            return QColor(241, 196, 15);  // yellow
        case TicketPriority::TICKET_PRIORITY_LOW:
            return QColor(46, 204, 113);  // green
        case TicketPriority::TICKET_PRIORITY_TRIVIAL:
            return QColor(127, 140, 141);  // gray
        case TicketPriority::TICKET_PRIORITY_NONE:
        default:
            return QColor(149, 165, 166);  // light gray
    }
}

struct SparePartsTable
{
    TicketSparePartUsedInformation spareData{};
    std::string articleName {};
};

struct SimilarReport
{
    std::uint64_t ticketID;
    std::string title;
};

struct CompanyContactEntry
{
    std::uint32_t id = 0;
    std::uint32_t companyId = 0;

    QString companyName;
    QString contactPerson;
    QString mailAddress;
    QString phoneNumber;

    QString address;
    QString street;
    QString houseNumber;
    QString postalCode;
    QString city;
    QString country;

    QString responsibleFor;
    QString customerNumber;
    QString moreInformation;
    QString website;

    std::uint8_t highlight = 0;
    std::uint8_t isDeleted = 0;
};
