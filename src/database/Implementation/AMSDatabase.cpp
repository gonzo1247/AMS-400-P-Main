#include "Implementation/AMSDatabase.h"

#include "PreparedStatementRegistry.h"

namespace database::Implementation
{
void RegisterAMSPreparedStatements()
{
    PREPARE_STATEMENT(AMSPreparedStatement::ACCOUNT_TEST_PING,
        "SELECT 1",
        CONNECTION_SYNC);

    // Ticket table

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_INSERT_NEW_TICKET, "INSERT INTO tickets "
    "(creator_user_id, current_status, cost_unit_id, area, reporter_name, reporter_phone, reporter_id, entity_id, title, "
    "description, priority) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
    CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_SELECT_ALL_TICKETS, "SELECT creator_user_id, created_at, current_status, cost_unit_id, area, reporter_id, entity_id, "
    "title, description, priority, updated_at, closed_at, last_status_change_at FROM tickets WHERE ID = ?", CONNECTION_SYNC);

PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT,
          "SELECT "
          "t.ID, "
          "t.title, "
          "t.area, "
          "t.created_at, "
          "t.updated_at, "
          "t.current_status, "
          "t.priority, "
          "t.cost_unit_id, "
          "ci.name          AS reporter_name, "
          "ci.phone         AS reporter_phone, "
          "ml.MachineName   AS machine_name, "
          "ep.firstName     AS employee_first_name, "
          "ep.lastName      AS employee_last_name, "
          "ep.phone         AS employee_phone, "
          "tc_latest.message AS latest_comment_message "
          "FROM tickets t "
          "LEFT JOIN caller_information ci ON ci.ID = t.reporter_id "
          "LEFT JOIN machine_list ml       ON ml.ID = t.entity_id "
          "LEFT JOIN ticket_assignment ta  ON ta.ticket_id = t.ID AND ta.is_current = 1 "
          "LEFT JOIN employees ep          ON ep.ID = ta.employee_id "
          "LEFT JOIN ("
          "SELECT tc.ticket_id, tc.message "
          "FROM ticket_comments tc "
          "INNER JOIN ("
          "SELECT ticket_id, MAX(COALESCE(updated_at, created_at)) AS max_last_at "
          "FROM ticket_comments "
          "WHERE is_deleted = 0 "
          "GROUP BY ticket_id"
          ") x ON x.ticket_id = tc.ticket_id "
          "AND x.max_last_at = COALESCE(tc.updated_at, tc.created_at) "
          "INNER JOIN ("
          "SELECT ticket_id, COALESCE(updated_at, created_at) AS last_at, MAX(id) AS max_id "
          "FROM ticket_comments "
          "WHERE is_deleted = 0 "
          "GROUP BY ticket_id, COALESCE(updated_at, created_at)"
          ") y ON y.ticket_id = tc.ticket_id "
          "AND y.last_at = COALESCE(tc.updated_at, tc.created_at) "
          "AND y.max_id = tc.id "
          "WHERE tc.is_deleted = 0"
          ") tc_latest ON tc_latest.ticket_id = t.ID "
          "WHERE t.is_deleted = 0 AND t.current_status != 7; ", CONNECTION_SYNC);


    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT_SINCE,
          "SELECT "
          "t.ID, "
          "t.title, "
          "t.area, "
          "t.created_at, "
          "t.updated_at, "
          "t.current_status, "
          "t.priority, "
          "t.cost_unit_id, "
          "ci.name          AS reporter_name, "
          "ci.phone         AS reporter_phone, "
          "ml.MachineName   AS machine_name, "
          "ep.firstName     AS employee_first_name, "
          "ep.lastName      AS employee_last_name, "
          "ep.phone         AS employee_phone "
          "FROM tickets t "
          "LEFT JOIN caller_information ci ON ci.ID = t.reporter_id "
          "LEFT JOIN machine_list ml       ON ml.ID = t.entity_id "
          "LEFT JOIN ticket_assignment ta  ON ta.ticket_id = t.ID AND ta.is_current = 1 "
          "LEFT JOIN employees ep          ON ep.ID = ta.employee_id "
          "WHERE t.is_deleted = 0 "
          "AND t.updated_at > ?; ",
          CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT_REMOVED_SINCE,
          "SELECT "
          "t.ID "
          "FROM tickets t "
          "WHERE t.updated_at > ? "
          "AND (t.is_deleted = 1 OR t.current_status IN (?, ?)); ",
          CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT_LAST_COMMENT_BY_ID, "SELECT message FROM ticket_comments WHERE ticket_id = ? AND is_deleted = 0 "
    "ORDER BY COALESCE(updated_at, created_at) DESC, id DESC LIMIT 1", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_STATUS_BY_ID, "UPDATE tickets SET current_status = ? WHERE ID = ?", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_PRIORITY_BY_ID, "UPDATE tickets SET priority = ? WHERE ID = ?", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_TITLE_BY_ID, "UPDATE tickets SET title = ? WHERE ID = ?", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_DESCRIPTION_BY_ID, "UPDATE tickets SET description = ? WHERE ID = ?", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_TO_CLOSED_BY_ID, "UPDATE tickets Set current_status = ?, closed_at = ? WHERE ID = ?", CONNECTION_ASYNC);

    // CI - caller_information
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_SELECT_ALL_CALLERS,
        "SELECT id, department, phone, name, costUnit, location, is_active "
        "FROM caller_information LIMIT 100",
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_INSERT_NEW_CALLER, 
        "INSERT INTO caller_information (department, phone, name, costUnit, location, is_active) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_SELECT_CALLER_BY_ID, "SELECT id, department, phone, name, costUnit, location, is_active FROM caller_information WHERE id = ?", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_SELECT_CALLER_AFTER_INSERT,
        "SELECT id FROM caller_information WHERE phone = ? AND name = ? AND location = ? AND is_active = 1", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_DELETE_CALLER_BY_ID, "UPDATE caller_information SET is_active = 0 WHERE id = ?", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_CI_UPDATE_CALLER_BY_ID,
        "UPDATE caller_information SET department = ?, phone = ?, name = ?, costUnit = ?, location = ?, "
        "is_active = ? WHERE id = ?",
        CONNECTION_SYNC);

    // EI - employee_information
    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_SELECT_ALL_EMPLOYEES, 
        "SELECT id, firstName, lastName, phone, location, isActive FROM employees", 
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_INSERT_EMPLOYEE, 
        "INSERT INTO employees (firstName, lastName, phone, location, isActive) "
        "VALUES (?, ?, ?, ?, ?)", 
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_UPDATE_EMPLOYEE_BY_ID,
        "UPDATE employees SET firstName = ?, lastName = ?, phone = ?, location = ?, isActive = ? WHERE id = ?",
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_SELECT_EMPLOYEE_AFTER_INSERT, 
        "SELECT id FROM employees WHERE firstName = ? AND lastName = ? AND phone = ? AND location = ? AND isActive = 1", 
        CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_SELECT_EMPLOYEE_BY_ID, "SELECT id, firstName, lastName, phone, location, isActive FROM employees WHERE id = ?",  CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_EI_DELETE_EMPLOYEE_BY_ID, 
        "DELETE FROM employees WHERE id = ?", 
        CONNECTION_SYNC);

    // machine list
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_ALL_MACHINES,
        "SELECT ID, CostUnitID, MachineTypeID, LineID, ManufacturerID, MachineName, MachineNumber, ManufacturerMachineNumber, RoomNumber, MoreInformation, location, is_deleted, deleted_at FROM machine_list", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_MACHINE_BY_ID, "SELECT ID, CostUnitID, MachineTypeID, LineID, ManufacturerID, MachineName, MachineNumber, ManufacturerMachineNumber, "
    "RoomNumber, MoreInformation, location FROM machine_list WHERE ID = ?", CONNECTION_SYNC);

    // Misc
    PREPARE_STATEMENT(AMSPreparedStatement::DB_SELECT_LAST_INSERT_ID, "SELECT LAST_INSERT_ID()", CONNECTION_SYNC);
    // 0: title, 1: description
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_SELECT_TITLE_DESC_BY_ID,
                      "SELECT title, description, entity_id FROM tickets WHERE ID = ? AND is_deleted = 0", CONNECTION_SYNC);

    // 0: report_plain
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TICKET_REPORT_SELECT_PLAIN_BY_TICKET_ID,
                      "SELECT report_plain FROM ticket_report WHERE ticket_id = ?", CONNECTION_SYNC);

    // ticket_assignment table
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TA_INSERT_NEW_TICKET_ASSIGNMENT, "INSERT INTO ticket_assignment (ticket_id, employee_id, assigned_at) VALUES (?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TA_INSERT_NEW_ASSIGNMENT_WITH_COMMENT, "INSERT INTO ticket_assignment (ticket_id, employee_id, assigned_at, comment_assigned, assigned_by_user_id) "
    "VALUES (?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TA_SELECT_TICKET_ASSIGNMENTS_BY_TICKET_ID,
                      "SELECT ticket_id, employee_id, assigned_at, unassigned_at, is_current, comment_assigned, comment_unassigned, assigned_by_user_id, unassigned_by_user_id "
                      "FROM ticket_assignment WHERE ticket_id = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TA_SELECT_CURRENT_TICKET_ASSIGNMENT_BY_TICKET_ID, "SELECT employee_id FROM ticket_assignment WHERE ticket_id = ? AND is_current = 1", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TA_DELETE_EMPLOYEE_ASSIGNMENT, "UPDATE ticket_assignment SET unassigned_at = ?, is_current = 0, comment_unassigned = ?, unassigned_by_user_id = ? "
    "WHERE ticket_id = ? AND employee_id = ?",
    CONNECTION_SYNC);

    // ticket_attachment table
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TATT_INSERT_NEW_TICKET_ATTACHMENT, "INSERT INTO ticket_attachments (ticket_id, uploaded_by_user, uploaded_at, original_filename, stored_filename, "
    "file_path, mime_type, file_size, description, is_deleted) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TATT_INSERT_EMPTY, "INSERT INTO ticket_attachments (ticket_id, uploaded_by_user, uploaded_at, original_filename, description, file_size, sha256_original, stored_filename, "
    "file_path, mime_type, encrypted_size, sha256_encrypted, is_deleted) VALUES "
    "(?, ?, ?, ?, ?, ?, ?, '', '', '', 0, '', 0)", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TATT_UPDATE_META, "UPDATE ticket_attachments SET stored_filename = ?, file_path = ?, encrypted_size = ?, sha256_encrypted = ?, mime_type = ? WHERE id = ?", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TATT_SELECT_TICKET_ATTACHMENTS_BY_TICKET_ID,
        "SELECT id, ticket_id, uploaded_by_user, uploaded_at, original_filename, stored_filename, file_path, mime_type, file_size, description, is_deleted "
        "FROM ticket_attachments WHERE ticket_id = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TATT_SELECT_BY_ID, "SELECT id, ticket_id, uploaded_by_user, uploaded_at, original_filename, stored_filename, file_path, mime_type, file_size, description, "
    "encrypted_size, sha256_original, sha256_encrypted, is_deleted FROM ticket_attachments WHERE id = ? LIMIT 1", CONNECTION_SYNC);

    // ticket_comment table
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TC_INSERT_NEW_TICKET_COMMENT, "INSERT INTO ticket_comments (ticket_id, author_user_id, created_at, is_internal, message) VALUES "
    "(?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TC_SELECT_TICKET_COMMENTS_BY_TICKET_ID,
        "SELECT id, ticket_id, author_user_id, created_at, updated_at, is_internal, is_deleted, message, delete_user_id, delete_at "
        "FROM ticket_comments WHERE ticket_id = ?", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TC_UPDATE_COMMENT_MARK_AS_DELETED, "UPDATE ticket_comments SET is_deleted = 1, delete_user_id = ?, delete_at = ? WHERE ID = ?", CONNECTION_SYNC);

    // ticket_status_history table
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TSH_SELECT_TICKET_STATUS_HISTORY_BY_TICKET_ID,
        "SELECT id, ticket_id, old_status, new_status, changed_at, changed_by_user, comment FROM ticket_status_history WHERE ticket_id = ?", CONNECTION_SYNC);

    // crypto_keys
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CS_SELECT_BY_NAME, "SELECT Algorithm, EncryptedKey FROM crypto_keys WHERE KeyName = ? LIMIT 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CS_UPDATE_KEY, "UPDATE crypto_keys SET KeyVersion = ?, EncryptedKey = ?, Algorithm = ?, UpdatedAt = ? WHERE KeyName = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CS_INSERT_NEW_KEY, "INSERT INTO crypto_keys (KeyName, KeyVersion, EncryptedKey, Algorithm, CreatedAt, UpdatedAt) VALUES (?, ?, ?, ?, ?, ?)", CONNECTION_SYNC);

    // contractor_visit CV
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CV_INSERT_NEW_CONTRACTOR_VISIT, "INSERT INTO contractor_visit (company_name, company_external_id, company_source, contact_person, arrival_at, "
                    "departure_at, activity, location, reachable_phone, reachable_note, status, note, created_by_user_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ? ,? ,? , ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CV_SELECT_CONTRACTOR_VISITS, "SELECT id, company_name, company_external_id, contact_person, worker_names_manual, arrival_at, departure_at, "
    "activity, location, reachable_phone, reachable_note, status, note, created_by_user_id, created_at, updated_at FROM contractor_visit", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CV_SELECT_OVERVIEW, "SELECT v.id, v.company_name, v.company_external_id, v.contact_person, v.worker_names_manual, v.arrival_at, "
                      "v.departure_at, v.activity, v.location, v.reachable_phone, v.reachable_note, v.status, v.note, v.created_by_user_id, v.created_at, v.updated_at "
                      "FROM contractor_visit v WHERE (v.arrival_at >= CURDATE()) OR (v.departure_at IS NULL) ORDER BY (v.departure_at IS NULL) DESC, v.arrival_at ASC", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CV_SELECT_BY_ID, "SELECT id, company_name, company_external_id, company_source, contact_person, arrival_at, "
                    "departure_at, activity, location, reachable_phone, reachable_note, status, note, created_by_user_id, created_at, updated_at "
                      "FROM contractor_visit WHERE id = ?", CONNECTION_SYNC); 
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CV_UPDATE_CONTRACTOR_VISIT_BY_ID, "UPDATE contractor_visit SET contact_person = ?, arrival_at = ?, departure_at = ?, activity = ?, location = ?, "
                    "reachable_phone = ?, reachable_note = ?, status = ?, note = ? WHERE id = ?", CONNECTION_SYNC);


    // ticket_spare_parts_used TSPU
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TSPU_INSERT_NEW_SPARE_PART_USED, "INSERT INTO ticket_spare_part_used (ticket_id, machine_id, article_id, quantity, unit, note, "
    "created_by, created_by_name) VALUES (?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TSPU_SELECT_SPARE_PARTS_USED_BY_TICKET_ID,
        "SELECT id, ticket_id, machine_id, article_id, quantity, unit, note, created_by, created_by_name, created_at "
        "FROM ticket_spare_part_used WHERE ticket_id = ? AND is_deleted = 0",
        CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TSPU_SOFT_DELETE_BY_ID, "UPDATE ticket_spare_part_used SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
        "WHERE id = ? AND is_deleted = 0", CONNECTION_SYNC);

    // ticket_report TR
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TR_INSERT_NEW_TICKET_REPORT,
                      "INSERT INTO ticket_report (ticket_id, report_html, report_plain, created_by, created_by_name) VALUES (?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_TR_SELECT_TICKET_REPORTS_BY_TICKET_ID,
        "SELECT id, ticket_id, report_html, report_plain, created_by, created_by_name, created_at, updated_at FROM ticket_report WHERE ticket_id = ?", CONNECTION_SYNC);

    PREPARE_STATEMENT(AMSPreparedStatement::DB_TR_UPDATE_TICKET_REPORT_BY_ID,
                      "UPDATE ticket_report SET report_html = ?, report_plain = ? WHERE id = ?", CONNECTION_ASYNC);

    // machine_line ML
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_ALL_LINES, "SELECT ID, Line, is_deleted, deleted_at FROM machine_line WHERE is_deleted = ? AND location = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_ALL_LINES_OVER_LOCATIONS, "SELECT ID, Line, is_deleted, deleted_at, location FROM machine_line WHERE is_deleted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_LINE_BY_ID, "SELECT ID, Line FROM machine_line WHERE ID = ? AND is_deleted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_INSERT_NEW_LINE, "INSERT INTO machine_line (Line, location) VALUES (?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_UPDATE_LINE_BY_ID, "UPDATE machine_line SET Line = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_DELETE_LINE_BY_ID, "UPDATE machine_line SET is_deleted = 1, deleted_at = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_UPDATE_RESTORE_LINE_BY_ID, "UPDATE machine_line SET is_deleted = 0, deleted_at = NULL WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_CAN_DELETE_LINE_BY_ID, "SELECT ID FROM machine_list WHERE LineID = ? LIMIT 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_ML_SELECT_NAME_BY_ID, "SELECT Line FROM machine_line WHERE ID = ?", CONNECTION_SYNC);

    // machine_type MT
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_SELECT_ALL_TYPES, "SELECT ID, Type, is_deleted, deleted_at FROM machine_type WHERE is_deleted = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_SELECT_TYPE_BY_ID, "SELECT ID, Type FROM machine_type WHERE ID = ? AND is_deleted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_SELECT_CAN_DELETE_TYPE_BY_ID, "SELECT ID FROM machine_list WHERE MachineTypeID = ? LIMIT 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_SELECT_NAME_BY_ID, "SELECT Type FROM machine_type WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_INSERT_NEW_TYPE, "INSERT INTO machine_type (Type) VALUES (?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_UPDATE_TYPE_BY_ID, "UPDATE machine_type SET Type = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_DELETE_TYPE_BY_ID, "UPDATE machine_type SET is_deleted = 1, deleted_at = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MT_UPDATE_RESTORE_TYPE_BY_ID, "UPDATE machine_type SET is_deleted = 0, deleted_at = NULL WHERE ID = ?", CONNECTION_SYNC);

    // machine_manufacturer MM
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_SELECT_ALL_MANUFACTURERS, "SELECT ID, Name, is_deleted, deleted_at FROM machine_manufacturer WHERE is_deleted = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_SELECT_MANUFACTURER_BY_ID, "SELECT ID, Name FROM machine_manufacturer WHERE ID = ? AND is_deleted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_SELECT_CAN_DELETE_MANUFACTURER_BY_ID, "SELECT ID FROM machine_list WHERE ManufacturerID = ? LIMIT 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_SELECT_NAME_BY_ID, "SELECT Name FROM machine_manufacturer WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_INSERT_NEW_MANUFACTURER, "INSERT INTO machine_manufacturer (Name) VALUES (?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_UPDATE_MANUFACTURER_BY_ID, "UPDATE machine_manufacturer SET Name = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_UPDATE_RESTORE_MANUFACTURER_BY_ID, "UPDATE machine_manufacturer SET is_deleted = 0, deleted_at = NULL WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_MM_DELETE_MANUFACTURER_BY_ID, "UPDATE machine_manufacturer SET is_deleted = 1, deleted_at = ? WHERE ID = ?", CONNECTION_SYNC);

    // facility_room FR
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_SELECT_ROOMS_BY_LOCATION, "SELECT ID, room_code, room_name, is_deleted, deleted_at FROM facility_room WHERE companyLocation = ? "
    "AND is_deleted = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_SELECT_ROOM_BY_LOCATION_ONLY, "SELECT ID, room_code, room_name FROM facility_room WHERE companyLocation = ? AND is_deleted = 0",
                      CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_SELECT_ALL_ROOMS, "SELECT ID, room_code, room_name, is_deleted, deleted_at, companyLocation FROM facility_room", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_SELECT_ROOM_BY_ID, "SELECT ID, room_code, room_name FROM facility_room WHERE ID = ? AND is_deleted = 0", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_SELECT_CAN_DELETE_ROOM_BY_ID, "SELECT ID FROM machine_list WHERE RoomNumber = ? LIMIT 1", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_INSERT_NEW_ROOM, "INSERT INTO facility_room (room_code, room_name, companyLocation) VALUES (?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_UPDATE_ROOM_BY_ID, "UPDATE facility_room SET room_code = ?, room_name = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_DELETE_ROOM_BY_ID, "UPDATE facility_room SET is_deleted = 1, deleted_at = ? WHERE ID = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_FR_UPDATE_RESTORE_ROOM_BY_ID, "UPDATE facility_room SET is_deleted = 0, deleted_at = NULL WHERE ID = ?", CONNECTION_SYNC);

    // contractor_worker CW
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CW_SELECT_ALL_CONTRACTOR_WORKERS, "SELECT id, company_name, company_external_id, first_name, last_name, phone, email, note, is_active, "
                    "created_at, updated_at FROM contractor_worker", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CW_INSERT_NEW_CONTRACTOR_WORKER, "INSERT INTO contractor_worker (company_name, company_external_id, first_name, last_name, phone, email, note, is_active) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CW_UPDATE_CONTRACTOR_WORKER_BY_ID,
                      "UPDATE contractor_worker SET company_name = ?, company_external_id = ?, first_name = ?, last_name = ?, phone = ?, email = ?, note = ?, is_active = ? WHERE id = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CW_UPDATE_DELETE_CONTRACTOR_WORKER_BY_ID, "UPDATE contractor_worker SET is_active = 0 WHERE id = ?", CONNECTION_SYNC);

    // contractor_visit_worker CVW
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CVW_SELECT_WORKERS_BY_CONTRACTOR_VISIT_ID, "SELECT cvw.contractor_worker_id, cw.company_name, cw.company_external_id, cw.first_name, cw.last_name, "
                        "cw.phone, cw.email "
                        "FROM contractor_visit_worker AS cvw "
                        "LEFT JOIN contractor_worker AS cw ON cw.id = cvw.contractor_worker_id "
                        " WHERE contractor_visit_id = ?", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CVW_REPLACE_WORKERS_FOR_CONTRACTOR_VISIT_ID, "INSERT INTO contractor_visit_worker (contractor_visit_id, contractor_worker_id) VALUES (?, ?) "
                        "ON DUPLICATE KEY UPDATE contractor_worker_id = contractor_worker_id", CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CVW_INSERT_NEW_CONTRACTOR_VISIT_WORKER, "INSERT INTO contractor_visit_worker (contractor_visit_id, contractor_worker_id) VALUES (?, ?)",
                      CONNECTION_SYNC);
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CVW_DELETE_WORKERS_FOR_CONTRACTOR_VISIT_ID, "DELETE FROM contractor_visit_worker WHERE contractor_visit_id = ? AND contractor_worker_id = ?", CONNECTION_SYNC);

    // contractor_visit_status CVS
    PREPARE_STATEMENT(AMSPreparedStatement::DB_CVS_SELECT_ALL_STATUS, "SELECT id, status_name, description FROM contractor_visit_status", CONNECTION_SYNC);
}

} // namespace database::Implementation
