#include "Implementation/IMSDatabase.h"

#include "PreparedStatementRegistry.h"

using namespace database::Implementation;

namespace database::Implementation
{
void RegisterIMSPreparedStatements()
{
    PREPARE_STATEMENT(IMSPreparedStatement::DB_VS_SELECT_ACTIVE_VERSION_INFO, "SELECT version_code, min_client_code, max_client_code, bypass_active, version_str "
                      "FROM version "
                      "WHERE component = ? AND is_active = 1 "
                      "LIMIT 1 ",
                      CONNECTION_SYNC);

    // CL - Company Location
    PREPARE_STATEMENT(IMSPreparedStatement::DB_CL_SELECT_ALL_COMPANY_LOCATIONS, "SELECT ID, location, FullName FROM company_locations", CONNECTION_SYNC);

    // SR - storage_rooms
    PREPARE_STATEMENT(IMSPreparedStatement::DB_SR_SELECT_ALL_ROOMS, "SELECT ID, RoomName FROM storage_rooms WHERE CompanyLocation = ? ORDER BY ID ASC", CONNECTION_SYNC);

	// RBAC
    PREPARE_STATEMENT(
        IMSPreparedStatement::DB_RBAC_SELECT_ACCOUNT_PERMISSION, "SELECT COUNT(*) FROM rbac_account_permissions WHERE accountId = ? AND permissionId = ? AND granted = 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_GROUP_PERMISSION,
                     "SELECT COUNT(*) FROM rbac_group_permissions gp JOIN rbac_account_groups ag ON gp.groupId = "
                     "ag.groupId WHERE ag.accountId = ? AND gp.permissionId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_GROUP_PERMISSION_RECURSIVE,
                     "SELECT permissionId FROM rbac_group_permissions gp JOIN rbac_account_groups ag ON gp.groupId = "
                     "ag.groupId WHERE ag.accountId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_LINKED_PERMISSION, "SELECT linkedId FROM rbac_linked_permissions WHERE id = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_ACCOUNT_GROUPS, "SELECT groupId FROM rbac_account_groups WHERE accountId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_GROUP_PERMISSIONS, "SELECT permissionId FROM rbac_group_permissions WHERE groupId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_ACCOUNT_OVERRIDES, "SELECT permissionId, granted FROM rbac_account_permissions WHERE accountId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_AUDIT_INSERT, "INSERT INTO rbac_audit_log (accountId, action, timestamp) VALUES (?, ?, NOW())", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_GROUP_ROLE, "SELECT permissionId FROM rbac_group_permissions WHERE groupId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_ALL_ROLE_PERMISSIONS,
                     "SELECT gp.groupId, lp.linkedId "
                     "FROM rbac_group_permissions gp "
                     "LEFT JOIN rbac_linked_permissions lp ON gp.permissionId = lp.id "
                     "UNION "
                     "SELECT lp.id, lp.linkedId "
                     "FROM rbac_linked_permissions lp", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_PERMISSION_DENIED, "SELECT 1 FROM rbac_account_permissions WHERE accountId = ? AND permissionId = ? AND granted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_SELECT_PERMISSION_ALLOWED_OR_DENIED, "SELECT granted FROM rbac_account_permissions WHERE accountId = ? AND permissionId = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_DELETE_ACCOUNT_GROUP_BY_ID, "DELETE FROM rbac_account_groups WHERE accountId = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RBAC_DELETE_ACCOUNT_PERMISSIONS_BY_ID, "DELETE FROM rbac_account_permissions WHERE accountId = ?", CONNECTION_BOTH);

    // RBAC RAP - rbac_account_permissions
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAP_DELETR_DATA_BY_PERMISSION_AND_USER_ID, "DELETE FROM rbac_account_permissions WHERE permissionId = ? AND accountId = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAP_INSERT_ACCOUNT_PERMISSION, "INSERT INTO rbac_account_permissions (permissionId, accountId, granted) VALUES (?, ?, ?)", CONNECTION_BOTH);

    // RBAC RAG - rbac_account_groups
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAG_INSERT_NEW_USER, "INSERT INTO rbac_account_groups (accountId, groupId) VALUES (?, ?)", CONNECTION_SYNC);

    // LogData
    PREPARE_STATEMENT(IMSPreparedStatement::DB_LOG_DATA_INSERT_NEW_LOG, "INSERT INTO logs (LogFlag, LogDate, InternalID, OriginalData, ChangedData, UserName) VALUES "
                    "(?, ?, ?, ?, ?, ?)", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_LOG_DATA_DELETE_OLDER_ENTRIES, "DELETE FROM logs WHERE LogDate <= ?", CONNECTION_BOTH);

    // User
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_INSERT_USER_NEW_USER, "INSERT INTO user (Username, Password, ChipID, Email, AccessRights) VALUES (?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_INSERT_NEW_USER_BY_CHIP_ID, "INSERT INTO user (Username, Password, ChipID, Email, AccessRights) VALUES (?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_BY_USERNAME, "SELECT Password, Email, AccessRights FROM user WHERE Username = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_FOR_LOGIN_BY_USERNAME, "SELECT ID, Password, Email, AccessRights, ChipID FROM user WHERE Username = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_EMAIL_BY_ID, "SELECT Email FROM user WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_ACCESS_RIGHT_BY_ID, "SELECT AccessRights FROM user WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_PASSWORD_BY_ID, "SELECT Password FROM user WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_SELECT_USER_BY_CHIP_ID, "SELECT ID, Username, Email, AccessRights FROM user WHERE ChipID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DELETE_BY_USERNAME, "DELETE FROM user WHERE Username = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_SELECT_USER_BY_ID, "SELECT u.Username, u.ChipID, u.Email, u.AccessRights, "
        "ud.FirstName, ud.LastName, ud.TextMailOrder, ud.TextMailOfferRequest, ud.SubjectOfferRequestMail, ud.SubjectOrderMail, ud.InternPhoneNumber, ud.PersonalStyle, ud.PersonalLanguage "
        "FROM user u "
        "LEFT JOIN user_data ud ON u.ID = ud.ID "
        "WHERE u.ID = ?", CONNECTION_SYNC);
    // New User Changes
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_FIRST_NAME_BY_USERID, "UPDATE user_data SET FirstName = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_LAST_NAME_BY_USERID, "UPDATE user_data SET LastName = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_CHANGE_USER_NAME, "UPDATE user SET Username = ?, Password = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_PASSWORD, "UPDATE user SET Password = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_MAIL_ADDRESS, "UPDATE user SET Email = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_ACCESS_RIGHT_BY_ID, "UPDATE user SET AccessRights = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAG_INSERT_RBAC_ACCESS, "INSERT INTO rbac_account_groups (accountId, groupId) VALUES (?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAG_DELETE_OLD_ACCESS, "DELETE FROM rbac_account_groups WHERE accountId = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_BODY_OFFER, "UPDATE user_data SET TextMailOfferRequest = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_SUBJECT_OFFER, "UPDATE user_data SET SubjectOfferRequestMail = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_BODY_ORDER, "UPDATE user_data SET TextMailOrder = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_MAIL_SUBJECT_ORDER, "UPDATE user_data SET SubjectOrderMail = ? WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DELETE_USER_BY_ID, "DELETE FROM user WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_DELETE_USER_DATA_BY_ID, "DELETE FROM user_data WHERE ID = ?", CONNECTION_BOTH);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_RAP_DELETE_DATA_BY_USER_ID, "DELETE FROM rbac_account_permissions WHERE accountId = ?", CONNECTION_BOTH);
    // User data
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_INSERT_USER_DATA_NEW_USER,
                     "INSERT INTO user_data (ID, FirstName, LastName, TextMailOrder, TextMailOfferRequest, "
                     "SubjectOfferRequestMail, SubjectOrderMail, "
                     " PersonalStyle, PersonalLanguage) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_INSERT_BASIC_DATA, "INSERT INTO user_data (ID, PersonalStyle, PersonalLanguage) VALUES (?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_INSERT_FIRST_LAST_NAME_COMBINE, "INSERT INTO user_data (ID, FirstName, LastName) VALUES (?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USER_DATA_BY_ID,
        "SELECT FirstName, LastName, TextMailOrder, TextMailOfferRequest, SubjectOfferRequestMail, SubjectOrderMail, "
        "PersonalStyle, PersonalLanguage, InternPhoneNumber FROM user_data WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_SELECT_FOR_EXIST, "SELECT ID FROM user_data WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_FIRST_LAST_NAME_COMBINE, "UPDATE user_data SET FirstName = ?, LastName = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_STYLE_BY_USERID, "UPDATE user_data SET PersonalStyle = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_DATA_UPDATE_PERSONAL_LANGUAGE_BY_USERID, "UPDATE user_data SET PersonalLanguage = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_DELETE_USER_DATA_BY_ID, "DELETE FROM user_data WHERE ID = ?", CONNECTION_SYNC);
    // Check before add new User
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_USERNAME_BY_USERNAME, "SELECT ID FROM user WHERE username = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_EMAIL_BY_EMAIL, "SELECT ID FROM user WHERE Email = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_ORDER_SELECT_ACCESS_RIGHT_BY_USERNAME, "SELECT AccessRights FROM user WHERE Username = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_SELECT_CHIP_ID, "SELECT Username FROM user WHERE ChipID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_SELECT_ON_START, "SELECT us.ID, us.Username, us.ChipID, us.Email, us.AccessRights, ud.PersonalStyle, "
                     "ud.PersonalLanguage, ud.FirstName, ud.LastName FROM user AS us "
                     "LEFT JOIN user_data AS ud ON us.ID = ud.ID LIMIT 10", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_RFID_BY_ID, "UPDATE user SET ChipID = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(IMSPreparedStatement::DB_USER_UPDATE_USER_ACCESS_RIGHT_BY_ID, "UPDATE user SET AccessRights = ? WHERE ID = ?", CONNECTION_SYNC);

    // MT - Message Translation
    PREPARE_STATEMENT(IMSPreparedStatement::DB_MT_SELECT_TRANSLATION_BY_LANGUAGE_AND_KEY, "SELECT message FROM message_translations WHERE locale = ? AND ID = ?", CONNECTION_SYNC);

    // CCU - company_cost_unit
    PREPARE_STATEMENT(IMSPreparedStatement::DB_CCU_SELECT_ALL_COST_UNITS, "SELECT ID, Locale, CostUnitID, CostUnitName, Place, Barcode FROM company_cost_units", CONNECTION_SYNC);
        
    // Global Settings
    PREPARE_STATEMENT(IMSPreparedStatement::DB_GS_LOAD_LOCAL_FILE_ROOT_PATH, "SELECT Data, DataType FROM global_settings WHERE SettingKey = ?", CONNECTION_ASYNC);

    // article_database AD
    PREPARE_STATEMENT(IMSPreparedStatement::DB_AD_SELECT_ARTICLE_NAME_BY_ID, "SELECT ArticleName FROM article_database WHERE ID = ?", CONNECTION_SYNC);

    // company_contact_data CCD
    PREPARE_STATEMENT(IMSPreparedStatement::DB_CCD_SELECT_ALL_COMPANY_CONTACTS, "SELECT ID, CompanyID, CompanyName, ContactPerson, MailAddress, PhoneNumber, Address, Street, HouseNumber, "
                    "PostalCode, City, Country, ResponsibleFor, CustomerNumber, MoreInformation, Website, highlight, IsDeleted FROM company_contact_data WHERE (? = 1 OR IsDeleted = 0)", CONNECTION_SYNC);

}

} // namespace database::Implementation
