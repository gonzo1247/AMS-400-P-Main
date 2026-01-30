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

#include <regex>

#include <QMessageBox>
#include <QFile>
#include <QStyleFactory>

#include "UserManagement.h"

#include "ConnectionGuard.h"
#include "MainFrame.h"
#include "RBACAccess.h"
#include "SettingsManager.h"

User::User() : _logDataMgr(std::make_unique<LogDataManager>())
{
	_userData = UserData();

	userPasswordHashes = ReadUserHashesFromFile();
}

std::string User::calculateSHA256(const QString& input)
{
	QByteArray byteArray = input.toUtf8();
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char*>(byteArray.constData()), byteArray.length(), hash);

	// Convert the hash into a hexadecimal std::string
	std::string result;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		result += QString("%1").arg(hash[i], 0, 16).rightJustified(2, '0').toStdString();

	return result;
}

std::string User::calculateSHA256(const std::string& input)
{
	SHA256_CTX sha256Context;
	SHA256_Init(&sha256Context);
	SHA256_Update(&sha256Context, input.c_str(), input.length());

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_Final(hash, &sha256Context);

	std::stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

	return ss.str();
}

bool User::UserLogin()
{
	std::string passwordHash = calculateSHA256(GenerateUsernameAndPasswordString(_username, _password));

	if (FindUserHash(passwordHash))
		return true;

	return false;
}

bool User::UserLogin(std::string username, std::string password)
{
	std::string passwordHash = calculateSHA256(GenerateUsernameAndPasswordString(username, password));
	if (FindUserHash(passwordHash))
		return true;

	return false;
}

bool User::AddNewUser(std::string username, std::string password)
{
	std::string newUserHash = calculateSHA256(GenerateUsernameAndPasswordString(username, password));

	if (FindUserHash(newUserHash))
	{
	//	_errorHandler->SendWarningDialog(ErrorCodes::WARNING_USER_ALREADY_EXIST);
		return false;
	}

	if (WriteUserHashesToFile(newUserHash))
	{
		ShowUserSuccesfullyCreated(username);
		return true;
	}

	return false;
}
ErrorCodes User::AddNewUser(std::uint32_t& newUserID, std::string username, std::string password, std::string email, AccessRights accessRights, std::string chipID /*= ""*/)
{
	if (username.empty() || password.empty())
		return ErrorCodes::WARNING_USER_NAME_TO_SHORT;

	ErrorCodes error = CheckDataBeforeAddNewUser(username, email, chipID);
	if (error != ErrorCodes::ERROR_NO_ERRORS)
		return error;

    ConnectionGuardIMS guard(ConnectionType::Sync);

	std::ranges::transform(username, username.begin(), [](unsigned char c) { return std::tolower(c); });

	auto newUser = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_INSERT_USER_NEW_USER);
	newUser->SetString(1, username);
	newUser->SetString(2, calculateSHA256(GenerateUsernameAndPasswordString(username, password)));
	newUser->SetString(3, chipID);
	newUser->SetString(4, email);
	newUser->SetInt(5, static_cast<std::uint8_t>(accessRights));

	if (!guard->ExecutePreparedInsert(*newUser))
		return ErrorCodes::ERROR_USER_WAS_NOT_CREATED_INTERN;

	// DB_ORDER_SELECT_USERNAME_BY_USERNAME, "SELECT ID FROM user WHERE username = ?");
	auto selectNewUserID = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USERNAME_BY_USERNAME);
	selectNewUserID->SetString(1, username);
	auto newUserResult = guard->ExecutePreparedSelect(*selectNewUserID);
	std::uint32_t userID = 0;

	if (newUserResult.IsValid())
		if (newUserResult.Next())
		{
            Field* fields = newUserResult.Fetch();
            userID = fields[0].GetUInt32();
		}

	if (userID >= 1)
	{
		// 	INSERT INTO user_data (ID, PersonalStyle, PersonalLanguage) VALUES (?, ?, ?)

		newUserID = userID;

		auto basicUserDataStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_INSERT_BASIC_DATA);
		basicUserDataStmt->SetUInt(1, userID);
		basicUserDataStmt->SetUInt(2, static_cast<std::uint32_t>(GetSettings().getStyleSelection()));
		basicUserDataStmt->SetString(3, GetSettings().getLanguage());
		if (!guard->ExecutePreparedInsert(*basicUserDataStmt))
			LOG_SQL("Can not add basic user data for user {} with ID {}", username, userID);

		// Add now user RBAC Data
		// INSERT INTO rbac_account_groups (accountId, groupId) VALUES (?, ?)

		auto rbacDataStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_RAG_INSERT_NEW_USER);
		rbacDataStmt->SetUInt(1, userID);
		rbacDataStmt->SetUInt(2, static_cast<std::uint8_t>(accessRights));

		if (!guard->ExecutePreparedInsert(*rbacDataStmt))
			LOG_SQL("Can not add RBAC Account Group for User {}", userID);

	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CREATE_USER, userID, "UserName: " + username);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::AddNewUserChip(std::uint32_t& newUserID, std::string chipID /*= ""*/, AccessRights accessRights /*= AccessRights::ACCESS_RIGHT_NONE*/, std::string username /*= ""*/, std::string password /*= ""*/, std::string email /*= ""*/)
{
	if (chipID.empty())
		return ErrorCodes::WARNING_NO_CHIP_ID;

	ErrorCodes error = CheckDataBeforeAddNewUser(username, email, chipID);
	if (error != ErrorCodes::ERROR_NO_ERRORS)
		return error;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	std::ranges::transform(username, username.begin(), [](unsigned char c) { return std::tolower(c); });

	// INSERT INTO user (Username, Password, ChipID, Email, AccessRights) VALUES (?, ?, ?, ?, ?)
	auto newUser = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_INSERT_NEW_USER_BY_CHIP_ID);
	newUser->SetString(1, username);
	if (!username.empty() && !password.empty())
		newUser->SetString(2, calculateSHA256(GenerateUsernameAndPasswordString(username, password)));
	else
		newUser->SetString(2, password);

	newUser->SetString(3, chipID);
	newUser->SetString(4, email);
	newUser->SetInt(5, static_cast<std::uint8_t>(accessRights));

	if (!guard->ExecutePreparedInsert(*newUser))
		return ErrorCodes::ERROR_USER_WAS_NOT_CREATED_INTERN;

	// DB_ORDER_SELECT_USERNAME_BY_USERNAME, "SELECT ID FROM user WHERE username = ?");
	auto selectNewUserID = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USERNAME_BY_USERNAME);
	selectNewUserID->SetString(1, username);
	auto newUserResult = guard->ExecutePreparedSelect(*selectNewUserID);
	std::uint32_t userID = 0;

	if (newUserResult.IsValid())
		if (newUserResult.Next())
		{
            Field* fields = newUserResult.Fetch();
			userID = fields[0].GetUInt32();
		}

	if (userID >= 1)
	{
		// 	INSERT INTO user_data (ID, PersonalStyle, PersonalLanguage) VALUES (?, ?, ?)

		newUserID = userID;

		auto basicUserDataStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_INSERT_BASIC_DATA);
		basicUserDataStmt->SetUInt(1, userID);
		basicUserDataStmt->SetUInt(2, static_cast<std::uint32_t>(GetSettings().getStyleSelection()));
		basicUserDataStmt->SetString(3, GetSettings().getLanguage());
		if (!guard->ExecutePreparedInsert(*basicUserDataStmt))
			LOG_SQL("Can not add basic user data for user {} with ID {}", username, userID);

		// Add now user RBAC Data
		// INSERT INTO rbac_account_groups (accountId, groupId) VALUES (?, ?)

		auto rbacDataStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_RAG_INSERT_NEW_USER);
		rbacDataStmt->SetUInt(1, userID);
		rbacDataStmt->SetUInt(2, static_cast<std::uint8_t>(accessRights));

		if (!guard->ExecutePreparedInsert(*rbacDataStmt))
			LOG_SQL("Can not add RBAC Account Group for User {}", userID);

	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CREATE_USER, userID, "UserName: " + username);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::AddAdditionalInfosToUser(std::uint32_t userID, std::string firstName, std::string lastName)
{
	ConnectionGuardIMS guard(ConnectionType::Sync);

	if (CheckIfUserDataEntryExist(userID))
	{
		// Update
		// PrepareStatement(DB_USER_DATA_UPDATE_FIRST_LAST_NAME_COMBINE, "UPDATE user_data SET FirstName = ?, LastName = ? WHERE ID = ?");
		auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_FIRST_LAST_NAME_COMBINE);
		stmt->SetString(1, firstName);
		stmt->SetString(2, lastName);
		stmt->SetUInt(3, userID);

		if (!guard->ExecutePreparedUpdate(*stmt))
		{
			LOG_SQL("AddAdditionalInfosToUser: Can not Update FirstName {} LastName {} for UserID {}", firstName, lastName, userID);
			return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
		}

		return ErrorCodes::ERROR_NO_ERRORS;
	}

	// insert
	// PrepareStatement(DB_USER_DATA_INSERT_FIRST_LAST_NAME_COMBINE, "INSERT INTO user_data (ID, FirstName, LastName) VALUES (?, ?, ?)");
	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_INSERT_FIRST_LAST_NAME_COMBINE);
	stmt->SetUInt(1, userID);
	stmt->SetString(2, firstName);
	stmt->SetString(3, lastName);

	if (!guard->ExecutePreparedInsert(*stmt))
	{
		LOG_SQL("AddAdditionalInfosToUser: Can not Insert FirstName {} LastName {} for UserID {}", firstName, lastName, userID);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

	return ErrorCodes::ERROR_NO_ERRORS;

}

ErrorCodes User::AddAdditionalMailInfoToUser(std::string username, std::string mailSubjectOffer /*= ""*/, std::string mailBodyOffer /*= ""*/, std::string mailSubjectOrder /*= ""*/, std::string mailBodyOrder /*= ""*/)
{
	return ErrorCodes::ERROR_NO_ERRORS;
}

bool User::FindUserHash(const std::string& targetHash)
{
	for (const auto& hash : userPasswordHashes)
		if (hash == targetHash) 
			return true;

	return false;
}

bool User::WriteUserHashesToFile(std::string hashToAdd)
{
	std::ofstream file(getPathToFile(), std::ios::app);
	if (file.is_open())
	{
		file << hashToAdd << std::endl;
		file.close();
		return true;
	}
	else
	{
		LOG_MISC("User::WriteUserHashesToFile >>> Error when opening the file for writing. <<<<<");
		return false;
	}

	return false;
}

bool User::ChangePassword(std::string username, std::string oldPassword, std::string newPassword)
{
	std::string oldUserHash = calculateSHA256(GenerateUsernameAndPasswordString(username, oldPassword));
	if (!FindUserHash(oldUserHash))
		return false;

	std::string newUserHash = calculateSHA256(GenerateUsernameAndPasswordString(username, newPassword));

	UpdateUserDataInFile(oldUserHash, newUserHash);

	ShowPasswordChangeSuccesfully();

	return true;
}

bool User::IsValidEmail(const std::string& email)
{
	const std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");

	return std::regex_match(email, emailRegex);
}

void User::CreateBlankUserDataFile()
{
	// Open the file in read mode (std::ios::out)
	std::ofstream file(getPathToFile());

	// generate Basis User Login
	// Admin
	// Admin2023!

	// Check whether the file was opened successfully
	if (!file.is_open())
	{
		// send error
		return;
	}

	file << calculateSHA256(GenerateUsernameAndPasswordString("Admin", "Admin2023!")) << std::endl;

	file.close();
}


void User::ShowUserSuccesfullyCreated(QString username)
{
	QMessageBox msgBox;
	msgBox.setText(translate("UserManagement", "User created."));
	QString infoText = translate("UserManagement", "The user ");
	infoText += username;
	infoText += translate("UserManagement", " was successfully created.");
	msgBox.setInformativeText(infoText);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.setWindowTitle(translate("UserManagement", "User"));
	msgBox.setWindowIcon(QIcon(":/icons/icon/order_system_icon_alternate.ico"));
	msgBox.setMinimumWidth(400);
	msgBox.setMinimumHeight(300);

	// Show the MessageBox and wait for the user's response
	int ret = msgBox.exec();

	if (ret == QMessageBox::Ok)
		msgBox.close();
}

void User::ShowUserSuccesfullyCreated(std::string /*username*/)
{

}

void User::UpdateUserDataInFile(std::string oldUserDataHash, std::string newUserDataHash)
{
	// Read the entire file content into a vector
	std::ifstream fileIn(getPathToFile());
	if (!fileIn.is_open()) 
		return;

	std::vector<std::string> lines;
	std::string line;
	while (std::getline(fileIn, line))
		lines.push_back(line);

	fileIn.close();

	// Search and replace the entry
	for (std::string& entry : lines)
	{
		size_t found = entry.find(oldUserDataHash);
		if (found != std::string::npos)
		{
			entry.replace(found, oldUserDataHash.length(), newUserDataHash);
			break; // Abort the loop after the first entry has been found
		}
	}

	// Write the updated data back to the file
	std::ofstream fileOut(getPathToFile());
	if (!fileOut.is_open())
		return;

	for (const std::string& entry : lines)
		fileOut << entry << std::endl;

	fileOut.close();

	// Update the stored vector
	userPasswordHashes = ReadUserHashesFromFile();

}

void User::ShowPasswordChangeSuccesfully()
{
	QMessageBox msgBox;
	msgBox.setText(translate("UserManagement", "Password change"));
	msgBox.setInformativeText(translate("UserManagement", "The password change was successful!"));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.setWindowTitle(translate("UserManagement", "Password"));
	msgBox.setWindowIcon(QIcon(":/icons/icon/order_system_icon_alternate.ico"));
	msgBox.setMinimumWidth(400);
	msgBox.setMinimumHeight(300);

	// Show the MessageBox and wait for the user's response
	int ret = msgBox.exec();

	if (ret == QMessageBox::Ok)
		msgBox.close();
}

std::string User::getPathToFile()
{
	std::filesystem::path programDataPath = std::filesystem::path(std::getenv("PROGRAMDATA")) / "Lager-und-Bestellverwaltung";
	std::filesystem::create_directories(programDataPath);
	std::filesystem::path currentPath = programDataPath / "userData.udat";

	return currentPath.string();
}

std::vector<std::string> User::ReadUserHashesFromFile()
{
	std::vector<std::string> hashes;
	std::ifstream file(getPathToFile());
	if (file.is_open())
	{
		std::string line;
		while (getline(file, line))
			hashes.push_back(line);

		file.close();
	}
	else
		CreateBlankUserDataFile();

	return hashes;
}


QString User::GenerateUsernameAndPasswordQString(std::string username, std::string password)
{
	QString userPassword = QString::fromStdString(username);
	userPassword += ":";
	userPassword += QString::fromStdString(password);
	return userPassword;
}

std::string User::GenerateUsernameAndPasswordString(std::string username, std::string password)
{
	std::string userPassword = username;
	userPassword += ":";
	userPassword += password;
	return userPassword;
}

bool User::UserLoginDB(std::string username, std::string password)
{
	if (username.empty() || password.empty())
		return false;

	std::ranges::transform(username, username.begin(), [](unsigned char c) { return std::tolower(c); });

	ConnectionGuardIMS guard(ConnectionType::Sync);
	//ConnectionGuardIMS _connection(ConnectionType::Sync);

	//		  1		2		3		4			5
	// SELECT ID, Password, Email AccessRights, ChipID FROM user WHERE Username = ?
	auto login = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USER_FOR_LOGIN_BY_USERNAME);
	login->SetString(0, username);
	auto loginResult = guard->ExecutePreparedSelect(*login);

	if (!loginResult.IsValid())
		return false;

	if (loginResult.Next())
	{
        Field* fields = loginResult.Fetch();
        std::string dbPassword = fields[1].GetString();

		std::string inputPasswordHash = calculateSHA256(GenerateUsernameAndPasswordString(username, password));

		// The password entered is not the same as the saved password
		if (dbPassword != inputPasswordHash)
			return false;

		_userData.userID = fields[0].GetUInt32();
		_userData.userName = username;
		_userData.userPassword = dbPassword;
		_userData.userEmail = fields[2].GetString();
		std::uint8_t rights = fields[3].GetUInt8();
		_userData.userAccessRights = static_cast<AccessRights>(rights);
		_userData.chipID = fields[4].GetString();

		_userIsLoggedIn = true;

		// If possible load extra user data
		//			1			2			3			4							5					6
		// SELECT FirstName, LastName, TextMailOrder, TextMailOfferRequest, SubjectOfferRequestMail, SubjectOrderMail, 
		//	7					8				9
		// PersonalStyle, PersonalLanguage, InternPhoneNumber FROM user_data WHERE ID = ?");

		auto extraUserData = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USER_DATA_BY_ID);
		extraUserData->SetUInt(0, _userData.userID);
		auto extraUserDataResult = guard->ExecutePreparedSelect(*extraUserData);

		if (extraUserDataResult.IsValid())
			if (extraUserDataResult.Next())
			{
                Field* fieldsExtra = extraUserDataResult.Fetch();
				_userData.userFirstName = fieldsExtra[0].GetString();
                _userData.userLastName = fieldsExtra[1].GetString();
                _userData.mailTextOrder = fieldsExtra[2].GetString();
                _userData.mailTextOffer = fieldsExtra[3].GetString();
                _userData.mailSubjectOffer = fieldsExtra[4].GetString();
                _userData.mailSubjectOrder = fieldsExtra[5].GetString();
                _userData.personalStyle = fieldsExtra[6].GetUInt8();
                _userData.personalLanguage = fieldsExtra[7].GetString();
                _userData.userPhoneNumber = fieldsExtra[8].GetString();
			}

/*
		if (GetSingeltonSettings()->GetIsPersonalStyleAllowed() && _userData.personalStyle != GetSingeltonSettings()->GetSelectedStyle())
		{
			GetMainFrameClass()->LoadPersonalStyle(_userData.personalStyle);
			_userData.changeStyleOrLanguage = true;
		}

		if (GetSingeltonSettings()->GetIsPersonalLanguageAllowed() && _userData.personalLanguage != GetSingeltonSettings()->GetLanguage())
		{
			GetMainFrameClass()->LoadPersonalTranslation(_userData.personalLanguage);
			_userData.changeStyleOrLanguage = true;
		}*/

		UserSingleton::getInstance().SetUserData(_userData);
		UserSingleton::getInstance().SetIsLoggedIn(true);
        RBACAccess::Initialize(_userData.userID);

		return true;
	}

	return false;
}

bool User::UserLoginChipID(std::string chipID /*= ""*/)
{
	if (chipID.empty())
		return false;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_SELECT_USER_BY_CHIP_ID);
	stmt->SetString(1, chipID);
	auto result = guard->ExecutePreparedSelect(*stmt);

	if (!result.IsValid())
		return false;
	
	if (result.Next())
	{
		// SELECT ID, Username, Email, AccessRights FROM user WHERE ChipID = ?
        Field* fields = result.Fetch();
		_userData.userID = fields[0].GetUInt32();
        _userData.userName = fields[1].GetString();
        _userData.userEmail = fields[2].GetString();
        std::uint8_t rights = fields[3].GetUInt8();
		_userData.userAccessRights = static_cast<AccessRights>(rights);
		_userData.chipID = chipID;
		_userIsLoggedIn = true;

		// If possible load extra user data
		//			1			2			3			4							5					6
		// SELECT FirstName, LastName, TextMailOrder, TextMailOfferRequest, SubjectOfferRequestMail, SubjectOrderMail, 
		//	7					8				9
		// PersonalStyle, PersonalLanguage, InternPhoneNumber FROM user_data WHERE ID = ?");

		auto extraUserData = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USER_DATA_BY_ID);
		extraUserData->SetUInt(1, _userData.userID);
		auto extraUserDataResult = guard->ExecutePreparedSelect(*extraUserData);

		if (extraUserDataResult.IsValid())
			if (extraUserDataResult.Next())
			{
                Field* fieldsExtra = extraUserDataResult.Fetch();
				_userData.userFirstName = fieldsExtra[0].GetString();
                _userData.userLastName = fieldsExtra[1].GetString();
                _userData.mailTextOrder = fieldsExtra[2].GetString();
				_userData.mailTextOffer = fieldsExtra[3].GetString();
				_userData.mailSubjectOffer = fieldsExtra[4].GetString();
				_userData.mailSubjectOrder = fieldsExtra[5].GetString();
				_userData.personalStyle = static_cast<std::uint8_t>(fieldsExtra[6].GetUInt8());
				_userData.personalLanguage = fieldsExtra[7].GetString();
				_userData.userPhoneNumber = fieldsExtra[8].GetString();
			}

/*
		if (GetSingeltonSettings()->GetIsPersonalStyleAllowed() && _userData.personalStyle != GetSingeltonSettings()->GetSelectedStyle())
		{
			GetMainFrameClass()->LoadPersonalStyle(_userData.personalStyle);
			_userData.changeStyleOrLanguage = true;
		}

		if (GetSingeltonSettings()->GetIsPersonalLanguageAllowed() && _userData.personalLanguage != GetSingeltonSettings()->GetLanguage())
		{
			GetMainFrameClass()->LoadPersonalTranslation(_userData.personalLanguage);
			_userData.changeStyleOrLanguage = true;
		}*/

		UserSingleton::getInstance().SetUserData(_userData);
		UserSingleton::getInstance().SetIsLoggedIn(true);
        RBACAccess::Initialize(_userData.userID);

		return true;
	}
	
	return false;
}

void User::UserLogout()
{
	_userData.userID = 0;
	_userData.chipID = "";
	_userData.userName = "";
	_userData.userPassword = "";
	_userData.userEmail = "";
	_userData.userAccessRights = AccessRights::ACCESS_RIGHT_NONE;
	_userIsLoggedIn = false;
	_userData.userFirstName = "";
	_userData.userLastName = "";
	_userData.personalStyle = 1;
	_userData.personalLanguage = "de_DE";
	_userData.changeStyleOrLanguage = false;
}

ErrorCodes User::ChangePasswordDB(std::uint32_t userID, std::string username, std::string oldPassword, std::string newPassword)
{
	ConnectionGuardIMS guard(ConnectionType::Sync);

	if (!GetUser().HasRBACPermission(
            Permission::RBAC_MISSING_OLD_REMOVE_BEFORE_RELEASE /*RBAC_CAN_CHANGE_PASSWORD_WITHOUT_OLD*/))
	{
		if (GetUser().GetUserID() == userID && oldPassword.empty())
			return ErrorCodes::WARNING_CHANGE_OWN_PASSWORD_NEED_OLD_PASSWORD;

		auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USER_PASSWORD_BY_ID);
		stmt->SetInt(1, userID);
		auto result = guard->ExecutePreparedSelect(*stmt);

		if (!result.IsValid())
			return ErrorCodes::WARNING_USER_PASSWORD_NOT_FOUND;

		if (result.Next())
		{
            Field* fields = result.Fetch();
			std::string oldPasswordString = fields[0].GetString();

			if (calculateSHA256(GenerateUsernameAndPasswordString(username, oldPassword)) != oldPasswordString)
				return ErrorCodes::WARNING_USER_OLD_PASSWORD_WRONG;
		}
	}

	// UPDATE user SET Password = ? WHERE ID = ?
	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_UPDATE_PASSWORD);
	stmt->SetString(1, calculateSHA256(GenerateUsernameAndPasswordString(username, newPassword)));
	stmt->SetUInt(2, userID);

	if (!guard->ExecutePreparedUpdate(*stmt))
	{
		LOG_MISC("ChangePasswordDB: Can not Update Password for User {}", userID);
		return ErrorCodes::ERROR_PASSWORD_CHANGE_DID_NOT_WORK;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Password for", "Username: " + username);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeMailAddress(std::uint32_t userID, std::string newMailAddress)
{
	if (!IsValidEmail(newMailAddress))
		return ErrorCodes::WARNING_NO_VALID_EMAIL_ADDRESS;

	ErrorCodes result = CheckIfEmailAlreadyInUse(newMailAddress);

	if (result != ErrorCodes::ERROR_NO_ERRORS)
		return result;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	// UPDATE user SET Email = ? WHERE ID = ?
	auto emailStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_UPDATE_MAIL_ADDRESS);
	emailStmt->SetString(1, newMailAddress);
	emailStmt->SetUInt(2, GetUser().GetUserID());

	if (!guard->ExecutePreparedUpdate(*emailStmt))
	{
		LOG_SQL("ChangeMailAddress: Can not change mail address from User: {} to new Mail Address: {}", userID, newMailAddress);
		return ErrorCodes::WARNING_EMAIL_NOT_CHANGED;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Mail Address Changed for", "User: " + Util::ConvertUint32ToString(userID));

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeRFIDData(std::uint32_t userID, std::string rfidData)
{
	ErrorCodes code = ErrorCodes::ERROR_NO_ERRORS;

	code = CheckIfRFIDInUse(rfidData);

	if (code != ErrorCodes::ERROR_NO_ERRORS)
		return code;

	ConnectionGuardIMS guard(ConnectionType::Sync);

	// RFID is not used, so the user can use it. Let's update them
	// UPDATE user SET ChipID = ? WHERE ID = ?
	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_UPDATE_RFID_BY_ID);
	stmt->SetString(1, rfidData);
	stmt->SetUInt(2, GetUser().GetUserID());

	if (!guard->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeRFIDData: Can not change RFID Data {} for userID {}", rfidData, userID);
		return ErrorCodes::WARNING_RFID_CAN_NOT_BE_CHANGED;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change ChipID for", "User: " + Util::ConvertUint32ToString(userID) + " to: " + rfidData);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeAccessRights(std::uint32_t userID, AccessRights rights)
{
	ConnectionGuardIMS guard(ConnectionType::Sync);

	// UPDATE user SET AccessRights = ? WHERE ID = ?
	auto rightStmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_UPDATE_USER_ACCESS_RIGHT_BY_ID);
	rightStmt->SetUInt(1, static_cast<std::uint32_t>(rights));
	rightStmt->SetUInt(2, GetUser().GetUserID());

	if (!guard->ExecutePreparedUpdate(*rightStmt))
	{
		LOG_SQL("ChangeAccessRights: Can not change Access Rights to {}  for user {}", static_cast<std::uint8_t>(rights), userID);
		return ErrorCodes::WARNING_ACCESS_RIGHT_NOT_CHANGED;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change AccessRight for ", "User " + Util::ConvertUint32ToString(userID) + " to Access Rights: " + Util::GetAccessRightStringName(rights));

	return UpdateRBACAccess(userID, rights);
}

ErrorCodes User::ChangeUserFirstName(std::uint32_t userID, std::string firstName)
{
	ConnectionGuardIMS guard(ConnectionType::Sync);

	// UPDATE user_data SET FirstName = ? WHERE ID = ?
	auto stmt = guard->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_FIRST_NAME_BY_USERID);
	stmt->SetString(1, firstName);
	stmt->SetUInt(2, userID);

	if (!guard->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeUserFirstName: Can not change FirstName {} for userID {}", firstName, userID);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change First Name from User: " + Util::ConvertUint32ToString(userID), "New First Name: " + firstName);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeUserLastName(std::uint32_t userID, std::string lastName)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// UPDATE user_data SET LastName = ? WHERE ID = ?
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_LAST_NAME_BY_USERID);
	stmt->SetString(1, lastName);
	stmt->SetUInt(2, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeUserLastName: Can not change LastName {} for userID {}", lastName, userID);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Last Name from User: " + Util::ConvertUint32ToString(userID), "New Last Name: " + lastName);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeUserName(std::uint32_t userID, std::string newUsername, std::string password)
{
    if (!GetUser().HasRBACPermission(Permission::RBAC_MISSING_OLD_REMOVE_BEFORE_RELEASE /*RBAC_CAN_CHANGE_USER_NAME*/))
		return ErrorCodes::ERROR_NO_ERRORS;

	ErrorCodes code = CheckIfUserNameExist(newUsername);
	if (code != ErrorCodes::ERROR_NO_ERRORS)
		return code;

	if (password.empty() || password.length() < 6)
		return ErrorCodes::WARNING_PASSWORD_TO_SHORT;

	// UPDATE user SET Username = ?, Password = ? WHERE ID = ?

	std::ranges::transform(newUsername, newUsername.begin(), [](unsigned char c) { return std::tolower(c); });

	ConnectionGuardIMS _connection(ConnectionType::Sync);

	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_UPDATE_CHANGE_USER_NAME);
	stmt->SetString(1, newUsername);
	stmt->SetString(2, calculateSHA256(GenerateUsernameAndPasswordString(newUsername, password)));
	stmt->SetUInt(3, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeUserName: User {} try to change Username from user {} to {} but it doesn't work!", GetUser().GetUserID(), userID, newUsername);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

	_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Username from User: " + Util::ConvertUint32ToString(userID), "New Username: " + newUsername);

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::UpdateRBACAccess(std::uint32_t userID, AccessRights rights)
{
    Permission permissionRole = RBACAccess::GetRBACRoleWithAccessRight(rights);

	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// First step: delete exist access
	// DELETE FROM rbac_account_groups WHERE accountId = ?
	auto deleteStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_RAG_DELETE_OLD_ACCESS);
	deleteStmt->SetUInt(1, userID);

	if (!_connection->ExecutePreparedUpdate(*deleteStmt))
	{
		LOG_SQL("UpdateRBACAccess: can not delete old access from database for user {}", userID);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

	// Second step: Add Access
	// INSERT INTO rbac_account_groups (accountId, groupId) VALUES (?, ?)
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_RAG_INSERT_RBAC_ACCESS);
	stmt->SetUInt(1, userID);
	stmt->SetUInt(2, static_cast<int>(permissionRole));

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("UpdateRBACAccess: can not add new access to database for user {}", userID);
		return ErrorCodes::ERROR_EXECUTE_SQL_PROBLEM;
	}

    RBACAccess::AuditGroupChange(GetUser().GetUserID(), userID, static_cast<std::uint32_t>(permissionRole));

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::DeleteUser(std::uint32_t userID)
{
	// Data must be deleted in the following tables: user, user_data, rbac_account_groups, rbac_account_permissions

	ConnectionGuardIMS _connection(ConnectionType::Sync);

	ErrorCodes result = ErrorCodes::ERROR_NO_ERRORS;

	// 1: Delete RAP - rbac_account_permissions
	auto rap = _connection->GetPreparedStatement(IMSPreparedStatement::DB_RAP_DELETE_DATA_BY_USER_ID);
	rap->SetUInt(1, userID);

	if (!_connection->ExecutePreparedDelete(*rap))
	{
		LOG_SQL("DeleteUser: Error on Delete RAP Data for UserID {}", userID);
		result = ErrorCodes::WARNING_USER_DELETE_SOMETHING_BE_WRONG;
	}

	// 2: Delete RAG - rbac_account_groups
	auto rag = _connection->GetPreparedStatement(IMSPreparedStatement::DB_RAG_DELETE_OLD_ACCESS);
	rag->SetUInt(1, userID);

	if (!_connection->ExecutePreparedDelete(*rag))
	{
		LOG_SQL("DeleteUser: Error in Delete RAG Data for UserID {}", userID);
		result = ErrorCodes::WARNING_USER_DELETE_SOMETHING_BE_WRONG;
	}

	// 3 Delete UD - user_data
	auto udDelete = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_DELETE_USER_DATA_BY_ID);
	udDelete->SetUInt(1, userID);

	if (!_connection->ExecutePreparedDelete(*udDelete))
	{
		LOG_SQL("DeleteUser: Error on Delete UD Data for UserID {}", userID);
		result = ErrorCodes::WARNING_USER_DELETE_SOMETHING_BE_WRONG;
	}

	// 4 Delete user
	auto user = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DELETE_USER_BY_ID);
	user->SetUInt(1, userID);

	if (!_connection->ExecutePreparedDelete(*user))
	{
		LOG_SQL("DeleteUser: Error on Delete user Data for UserID {}", userID);
		result = ErrorCodes::WARNING_USER_DELETE_SOMETHING_BE_WRONG;
	}

	return result;
}

ErrorCodes User::CheckDataBeforeAddNewUser(std::string username, std::string email, std::string rfid /*= ""*/)
{
	/*
		What needs to be checked:
		1) Is there already an account with the username?
		2) Is the email already in use? (Or should multiple use be possible?)
	*/
	ErrorCodes code = CheckIfUserNameExist(username);
	if (code != ErrorCodes::ERROR_NO_ERRORS)
		return code;

	code = CheckIfEmailAlreadyInUse(email);
	if (code != ErrorCodes::ERROR_NO_ERRORS)
		return code;

	if (!rfid.empty())
	{
		code = CheckIfRFIDInUse(rfid);
		if (code != ErrorCodes::ERROR_NO_ERRORS)
			return code;
	}

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::CheckIfUserNameExist(std::string username)
{
	std::ranges::transform(username, username.begin(), [](unsigned char c) { return std::tolower(c); });

	ConnectionGuardIMS _connection(ConnectionType::Sync);

	auto check = _connection->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_USERNAME_BY_USERNAME);
	check->SetString(1, username);
	auto checkResult = _connection->ExecutePreparedSelect(*check);

	if (checkResult.IsValid())
		if (checkResult.Next())
			return ErrorCodes::WARNING_USER_ALREADY_EXIST;

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::CheckIfEmailAlreadyInUse(std::string email)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	auto check = _connection->GetPreparedStatement(IMSPreparedStatement::DB_ORDER_SELECT_EMAIL_BY_EMAIL);
	check->SetString(1, email);
	auto checkResult = _connection->ExecutePreparedSelect(*check);

	if (checkResult.IsValid())
		if (checkResult.Next())
			return ErrorCodes::WARNING_EMAIL_ADDRESS_IN_USE;

	return ErrorCodes::ERROR_NO_ERRORS;
}


ErrorCodes User::CheckIfRFIDInUse(std::string rfid)
{
	// Check whether the RFID is already assigned to another user.
	// SELECT Username FROM user WHERE ChipID = ?

	ConnectionGuardIMS _connection(ConnectionType::Sync);

	auto checkStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_SELECT_CHIP_ID);
	checkStmt->SetString(1, rfid);
	auto checkResult = _connection->ExecutePreparedSelect(*checkStmt);

	if (checkResult.IsValid())
		if (checkResult.Next())
		{
            Field* fields = checkResult.Fetch();
            std::string existUser = fields[0].GetString();
			LOG_MISC("The RFID ID {} is already being used by user {}!", rfid, existUser);
			return ErrorCodes::WARNING_RFID_DATA_IN_USE;
		}

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangePersonalStyle(Styles styles)
{
	// First we need to check if user_data exist
	bool _userDataExists = ExistUserDataEntry(GetUser().GetUserID());

	if (_userDataExists)
	{
		ConnectionGuardIMS _connection(ConnectionType::Sync);

		// UPDATE user_data SET PersonalStyle = ? WHERE ID = ?
		auto updateStyle = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_STYLE_BY_USERID);
		updateStyle->SetUInt(1, static_cast<std::uint32_t>(styles));
		updateStyle->SetUInt(2, GetUser().GetUserID());

		if (!_connection->ExecutePreparedUpdate(*updateStyle))
		{
			LOG_SQL("User::ChangePersonalStyleAdmin: Can not change Personal Style for userID {}", GetUser().GetUserID());
			return ErrorCodes::ERROR_MYSQL_ERROR;
		}

		_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Personal Style", "Username: " + GetUser().GetUserName());

		return ErrorCodes::ERROR_NO_ERRORS;
	}

	// User not exist
	return InsertBasicUserData(GetUser().GetUserID(), styles);
}

ErrorCodes User::ChangePersonalStyle(std::uint32_t userID, Styles styles)
{
	// First we need to check if user_data exist
	bool _userDataExists = ExistUserDataEntry(userID);

	if (_userDataExists)
	{
		ConnectionGuardIMS _connection(ConnectionType::Sync);

		// UPDATE user_data SET PersonalStyle = ? WHERE ID = ?
		auto updateStyle = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_STYLE_BY_USERID);
		updateStyle->SetUInt(1, static_cast<std::uint32_t>(styles));
		updateStyle->SetUInt(2, userID);

		if (!_connection->ExecutePreparedUpdate(*updateStyle))
		{
			LOG_SQL("User::ChangePersonalStyle: Can not change Personal Style for userID {}", userID);
			return ErrorCodes::ERROR_MYSQL_ERROR;
		}

		_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Personal Style for", "User: " + Util::ConvertUint32ToString(userID));

		return ErrorCodes::ERROR_NO_ERRORS;
	}

	// User not exist
	return InsertBasicUserData(userID, styles);
}

ErrorCodes User::ChangePersonalLanguage(std::string language)
{
	// First we need to check if user_data exist
	bool _userDataExists = ExistUserDataEntry(GetUser().GetUserID());

	if (_userDataExists)
	{
		ConnectionGuardIMS _connection(ConnectionType::Sync);

		// UPDATE user_data SET PersonalLanguage = ? WHERE ID = ?
		auto updateStyle = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_LANGUAGE_BY_USERID);
		updateStyle->SetString(1, language);
		updateStyle->SetUInt(2, GetUser().GetUserID());

		if (!_connection->ExecutePreparedUpdate(*updateStyle))
		{
			LOG_SQL("User::ChangePersonalStyleAdmin: Can not change Personal Language for userID {}", GetUser().GetUserID());
			return ErrorCodes::ERROR_MYSQL_ERROR;
		}

		_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Personal Language", "Username: " + GetUser().GetUserName());

		return ErrorCodes::ERROR_NO_ERRORS;
	}

	// User not exist
	return InsertBasicUserData(GetUser().GetUserID(), language);
}

ErrorCodes User::ChangePersonalLanguage(std::uint32_t userID, SelectLanguage language)
{
	// First we need to check if user_data exist
	bool _userDataExists = ExistUserDataEntry(userID);

	if (_userDataExists)
	{
		ConnectionGuardIMS _connection(ConnectionType::Sync);

		// UPDATE user_data SET PersonalLanguage = ? WHERE ID = ?
		auto updateStyle = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_LANGUAGE_BY_USERID);
        updateStyle->SetString(1, GetSelectLanguageString(language));
        updateStyle->SetUInt(2, userID);

		if (!_connection->ExecutePreparedUpdate(*updateStyle))
		{
			LOG_SQL("User::ChangePersonalLanguage: Can not change Personal Language for userID {}", userID);
			return ErrorCodes::ERROR_MYSQL_ERROR;
		}

		_logDataMgr->WriteLogData(LogFilterFlags::LOG_FLAG_CHANGE_USER, GetUser().GetUserID(), "Change Personal Language for", "User: " + Util::ConvertUint32ToString(userID));

		return ErrorCodes::ERROR_NO_ERRORS;
	}

	// User not exist
	return InsertBasicUserData(userID, GetSelectLanguageString(language));
}

ErrorCodes User::ChangeMailBodyOffer(std::uint32_t userID, std::string mailBodyOffer)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// UPDATE user_data SET TextMailOfferRequest = ? WHERE ID = ?
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_BODY_OFFER);
	stmt->SetString(1, mailBodyOffer);
	stmt->SetUInt(2, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeMailBodyOffer: Can not change Mail Body for Offer Mail to {} for user {}", mailBodyOffer, userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return  ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeMailSubjectOffer(std::uint32_t userID, std::string mailSubjectOffer)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// UPDATE user_data SET SubjectOfferRequestMail = ? WHERE ID = ?
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_SUBJECT_OFFER);
	stmt->SetString(1, mailSubjectOffer);
	stmt->SetUInt(2, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeMailSubjectOffer: Can not change Mail Subject for Offer Mail to {} for user {}", mailSubjectOffer, userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return  ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeMailBodyOrder(std::uint32_t userID, std::string mailBodyOrder)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// UPDATE user_data SET TextMailOrder = ? WHERE ID = ?
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_BODY_ORDER);
	stmt->SetString(1, mailBodyOrder);
	stmt->SetUInt(2, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeMailBodyOrder: Can not change Mail Body for Order Mail to {} for user {}", mailBodyOrder, userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return  ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::ChangeMailSubjectOrder(std::uint32_t userID, std::string mailSubjectOrder)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// UPDATE user_data SET SubjectOrderMail = ? WHERE ID = ?
	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_SUBJECT_ORDER);
	stmt->SetString(1, mailSubjectOrder);
	stmt->SetUInt(2, userID);

	if (!_connection->ExecutePreparedUpdate(*stmt))
	{
		LOG_SQL("ChangeMailSubjectOrder: Can not change Mail Subject for Order Mail to {} for user {}", mailSubjectOrder, userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return  ErrorCodes::ERROR_NO_ERRORS;
}

bool User::CheckIfUserDataEntryExist(std::uint32_t userID)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	auto stmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_SELECT_FOR_EXIST);
	stmt->SetUInt(1, userID);
	auto result = _connection->ExecutePreparedSelect(*stmt);

	if (result.IsValid() && result.Next())
		return true;

	return false;
}

bool User::ExistUserDataEntry(std::uint32_t userID)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// SELECT ID FROM user_data WHERE ID = ?"
	auto checkStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_SELECT_FOR_EXIST);
	checkStmt->SetUInt(1, userID);
	auto checkResult = _connection->ExecutePreparedSelect(*checkStmt);

	if (checkResult.IsValid() && checkResult.Next())
		return true;

	return false;
}

ErrorCodes User::InsertBasicUserData(std::uint32_t userID, std::string language /*= "de_DE"*/, Styles styles /*= Styles::STYLE_LIGHT*/)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// INSERT INTO user_data (ID, PersonalStyle, PersonalLanguage) VALUES (?, ?, ?)"
	auto basicStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_INSERT_BASIC_DATA);
	basicStmt->SetUInt(1, userID);
	basicStmt->SetUInt(2, static_cast<std::uint32_t>(styles));
	basicStmt->SetString(3, language);

	if (!_connection->ExecutePreparedInsert(*basicStmt))
	{
		LOG_SQL("User::InsertBasicUserData: can not insert basic user data for userID {}", userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return ErrorCodes::ERROR_NO_ERRORS;
}

ErrorCodes User::InsertBasicUserData(std::uint32_t userID, Styles styles /*= Styles::STYLE_LIGHT*/, std::string language /*= "de_DE"*/)
{
	ConnectionGuardIMS _connection(ConnectionType::Sync);

	// INSERT INTO user_data (ID, PersonalStyle, PersonalLanguage) VALUES (?, ?, ?)"
	auto basicStmt = _connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_DATA_INSERT_BASIC_DATA);
    basicStmt->SetUInt(1, userID);
    basicStmt->SetUInt(2, static_cast<std::uint32_t>(styles));
    basicStmt->SetString(3, language);

	if (!_connection->ExecutePreparedInsert(*basicStmt))
	{
		LOG_SQL("User::InsertBasicUserData: can not insert basic user data for userID {}", userID);
		return ErrorCodes::ERROR_MYSQL_ERROR;
	}

	return ErrorCodes::ERROR_NO_ERRORS;
}

QString User::translate(const char* context, const char* sourceText, const char* disambiguation /*= nullptr*/, int n /*= -1*/)
{
	return QCoreApplication::translate(context, sourceText, disambiguation, n);
}

bool UserSingleton::HasUserAccessRight(QPushButton& button, AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
	{
		button.setVisible(true);
		return true;
	}
	else
	{
		button.setVisible(false);
		return false;
	}
}

bool UserSingleton::HasUserAccessRight(Qt::KeyboardModifiers modifier, int key, QObject* receiver, std::function<void()> slotFunction, AccessRights neededAccessRights) const
{
	bool hasAccess = _userData.userAccessRights >= neededAccessRights;
	Util::CreateAndConnectAction(modifier, key, receiver, slotFunction, hasAccess);
	return hasAccess;
}

bool UserSingleton::HasUserAccessRight(QLineEdit& edit, AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
	{
		edit.setVisible(true);
		return true;
	}
	else
	{
		edit.setVisible(false);
		return false;
	}
}

bool UserSingleton::HasUserAccessRight(QSpinBox& spin, AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
	{
		spin.setVisible(true);
		return true;
	}
	else
	{
		spin.setVisible(false);
		return false;
	}
}

bool UserSingleton::HasUserAccessRight(QTableWidget& table, AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
	{
		table.setVisible(true);
		return true;
	}
	else
	{
		table.setVisible(false);
		return false;
	}
}

bool UserSingleton::HasUserAccessRight(QComboBox& comboBox, AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
	{
		comboBox.setVisible(true);
		return true;
	}
	else
	{
		comboBox.setVisible(false);
		return false;
	}
}

bool UserSingleton::HasUserAccessRight(QLabel& label, Permission neededPermission, std::string text) const
{
    if (RBACAccess::HasPermission(static_cast<std::uint32_t>(neededPermission)))
	{
		label.setVisible(true);
		if (text != "")
			label.setText(QString::fromStdString(text));

		return true;
	}
	else
	{
		if (text != "")
			label.setText(QString::fromStdString(text));
		else
		{
			label.setVisible(false);
			return false;
		}
	}

	return false;
}

bool UserSingleton::HasUserAccessRight(AccessRights neededAccessRights /*= AccessRights::ACCESS_RIGHT_ADMIN*/) const
{
	if (_userData.userAccessRights >= neededAccessRights)
		return true;
	else
		return false;
}

bool UserSingleton::HasRBACPermission(Permission permission)
{
    return RBACAccess::HasPermission(static_cast<std::uint32_t>(permission));
}

void UserSingleton::UserLogout()
{
	_userData.userID = 0;
	_userData.userName = "";
	_userData.userPassword = "";
	_userData.userEmail = "";
	_userData.userAccessRights = AccessRights::ACCESS_RIGHT_NONE;
	_userData.userFirstName = "";
	_userData.userLastName = "";
	_userData.personalStyle = 1;
	_userData.personalLanguage = "de_DE";
	_userData.changeStyleOrLanguage = false;
	SetIsLoggedIn(false);
    RBACAccess::Shutdown();
}
