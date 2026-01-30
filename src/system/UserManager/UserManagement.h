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

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <vector>
#include <QSpinBox>

#include <QApplication>

#include "PermissionRBAC.h"
#include "SharedDefines.h"
#include "Util.h"
#include "LogDataManager.h"

class UIErrorHandler;

struct UserData
{
	std::uint32_t userID = 0;
	std::string userName{};
	std::string userPassword{};
	std::string chipID{};
	std::string userEmail{};
	AccessRights userAccessRights = AccessRights::ACCESS_RIGHT_NONE;
	std::string userFirstName{};
	std::string userLastName{};
	std::uint8_t personalStyle = 1;
	std::string personalLanguage = "de_DE";
	bool changeStyleOrLanguage = false;
	std::string mailSubjectOffer{};
	std::string mailTextOffer{};
	std::string mailSubjectOrder{};
	std::string mailTextOrder{};
	std::string userPhoneNumber{};

	std::uint32_t GetUserID() const { return userID; }
    const std::string& GetUserName() const { return userName; }
    const std::string& GetUserEmail() const { return userEmail; }
    const std::string& GetUserChipID() const { return chipID; }
    const std::string& GetUserFirstName() const { return userFirstName; }
    const std::string& GetUserLastName() const { return userLastName; }
    const std::string& GetUserPhone() const { return userPhoneNumber; }
    AccessRights GetAccessRights() const { return userAccessRights; }
};

class User
{
public:
	User();

	std::string calculateSHA256(const QString& input);
	std::string calculateSHA256(const std::string& input);

	bool UserLogin();
	bool UserLogin(std::string username, std::string password);
	bool AddNewUser(std::string username, std::string password);
	bool FindUserHash(const std::string& targetHash);
	bool WriteUserHashesToFile(std::string hashToAdd);
	bool ChangePassword(std::string username, std::string oldPassword, std::string newPassword);
	bool IsValidEmail(const std::string& email);

	void CreateBlankUserDataFile();
	void ShowUserSuccesfullyCreated(QString username);
	void ShowUserSuccesfullyCreated(std::string username);
	void UpdateUserDataInFile(std::string oldUserDataHash, std::string newUserDataHash);
	void ShowPasswordChangeSuccesfully();

	std::string getPathToFile();
	std::vector<std::string> ReadUserHashesFromFile();

	std::string GenerateUsernameAndPasswordString(std::string username, std::string password);
	QString GenerateUsernameAndPasswordQString(std::string username, std::string password);

	// Functions for User Database

	// Login / Logout
	bool UserLoginDB(std::string username, std::string password);
	bool UserLoginChipID(std::string chipID = "");
	void UserLogout();

	// Add New User
	ErrorCodes AddNewUser(std::uint32_t& newUserID, std::string username, std::string password, std::string email, AccessRights accessRights, std::string chipID = "");
	ErrorCodes AddNewUserChip(std::uint32_t& newUserID, std::string chipID = "", AccessRights accessRights = AccessRights::ACCESS_RIGHT_NONE, std::string username = "", std::string password = "", std::string email = "");
	ErrorCodes AddAdditionalInfosToUser(std::uint32_t userID, std::string firstName, std::string lastName);
	ErrorCodes AddAdditionalMailInfoToUser(std::string username, std::string mailSubjectOffer = "", std::string mailBodyOffer = "", std::string mailSubjectOrder = "", std::string mailBodyOrder = "");
	// New Changes
	ErrorCodes ChangeUserFirstName(std::uint32_t userID, std::string firstName);
	ErrorCodes ChangeUserLastName(std::uint32_t userID, std::string lastName);
	ErrorCodes ChangeUserName(std::uint32_t userID, std::string newUsername, std::string password);
	ErrorCodes ChangePasswordDB(std::uint32_t userID, std::string username, std::string oldPassword, std::string newPassword);
	ErrorCodes ChangeMailAddress(std::uint32_t userID, std::string newMailAddress);
	ErrorCodes ChangeRFIDData(std::uint32_t userID, std::string rfidData);
	ErrorCodes ChangeAccessRights(std::uint32_t userID, AccessRights rights);
	ErrorCodes ChangePersonalStyle(std::uint32_t userID, Styles styles);
	ErrorCodes ChangePersonalLanguage(std::uint32_t userID, SelectLanguage language);
	ErrorCodes ChangeMailBodyOffer(std::uint32_t userID, std::string mailBodyOffer);
	ErrorCodes ChangeMailSubjectOffer(std::uint32_t userID, std::string mailSubjectOffer);
	ErrorCodes ChangeMailBodyOrder(std::uint32_t userID, std::string mailBodyOrder);
	ErrorCodes ChangeMailSubjectOrder(std::uint32_t userID, std::string mailSubjectOrder);
	ErrorCodes UpdateRBACAccess(std::uint32_t userID, AccessRights rights);
	// Delete user
	ErrorCodes DeleteUser(std::uint32_t userID);

	// Helpers
	ErrorCodes CheckDataBeforeAddNewUser(std::string username, std::string email, std::string rfid = "");
	ErrorCodes CheckIfUserNameExist(std::string username);
	ErrorCodes CheckIfEmailAlreadyInUse(std::string email);
	ErrorCodes CheckIfRFIDInUse(std::string rfid);
	ErrorCodes ChangePersonalStyle(Styles styles);
	ErrorCodes ChangePersonalLanguage(std::string language);

	bool CheckIfUserDataEntryExist(std::uint32_t userID);

	bool ExistUserDataEntry(std::uint32_t userID);
	ErrorCodes InsertBasicUserData(std::uint32_t userID, std::string language = "de_DE", Styles styles = Styles::STYLE_LIGHT);
	ErrorCodes InsertBasicUserData(std::uint32_t userID, Styles styles = Styles::STYLE_LIGHT, std::string language = "de_DE");

	UserData GetUserData() { return _userData; }
	void SetUserDataChangeStyleOrLanguage(bool changed) { _userData.changeStyleOrLanguage = changed; }

private:
	// Functions for User Database


	static QString translate(const char* context, const char* sourceText, const char* disambiguation = nullptr, int n = -1);
	std::string _username;
	std::string _password;
	std::vector<std::string> userPasswordHashes;

	std::map<std::uint32_t, UserData> _userDataMap;
	UserData _userData;
	bool _userIsLoggedIn;

	std::unique_ptr<LogDataManager> _logDataMgr;
};

class UserSingleton
{
public:
	static UserSingleton& getInstance()
	{
		static UserSingleton instance;
		return instance;
	}

	const UserData& GetUserData() const { return _userData; }
	std::uint32_t GetUserID() const { return _userData.userID; }
	std::string GetUserChipID() const { return _userData.chipID; }
	std::string GetUserName() const { return _userData.userName; }
	std::string GetUserSName() const { return _userData.userName; }
	std::string GetUserEmail() const { return _userData.userEmail; }
	AccessRights GetUserAccessRights() const { return _userData.userAccessRights; }
	Styles GetUserPersonalStyle() const { return static_cast<Styles>(_userData.personalStyle); }
	std::string GetUserPersonalLanguage() const { return _userData.personalLanguage; }
	void SetUserDataChangeStyleOrLanguage(bool changed) { _userData.changeStyleOrLanguage = changed; }
	bool GetUserDataChangeStyleOrLanguage() const { return _userData.changeStyleOrLanguage; }

	bool HasUserAccessRight(QPushButton& button, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	bool HasUserAccessRight(QLineEdit& edit, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	bool HasUserAccessRight(QTableWidget& table, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	bool HasUserAccessRight(QComboBox& comboBox, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	bool HasUserAccessRight(Qt::KeyboardModifiers modifier, int key, QObject* receiver, std::function<void()> slotFunction, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	//label
	bool HasUserAccessRight(QLabel& label, Permission neededPermission, std::string text = "") const;
	
	bool HasUserAccessRight(AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;
	bool HasUserAccessRight(QSpinBox& spin, AccessRights neededAccessRights = AccessRights::ACCESS_RIGHT_ADMIN) const;

	bool HasRBACPermission(Permission permission);
	
	void SetUserData(const UserData userData) { _userData = userData; }
	void SetIsLoggedIn(bool isLogged) { _isLoggedIn = isLogged; }
	bool IsLoggedIn() const { return _isLoggedIn; }
	void UserLogout();

private:
	UserSingleton() = default;
	~UserSingleton() = default;

	UserData _userData;
	bool _isLoggedIn = false;
};

inline UserSingleton& GetUser()
{
	return UserSingleton::getInstance();
}