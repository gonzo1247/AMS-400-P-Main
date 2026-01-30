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

enum class Permission : int
{
	ROLE_NONE								= 0, // not exist
	ROLE_OBSERVER                           = 180,
	ROLE_OPERATOR                           = 181,
	ROLE_COORDINATOR                        = 182,
	ROLE_MANAGER                            = 183,
	ROLE_ADMIN								= 184,
	ROLE_MAX_ROLE							= 199,

	// Permissions
    // AMS start at 400
    RBAC_MISSING_OLD_REMOVE_BEFORE_RELEASE  = 399,
    RBAC_OPEN_CALLER_MANAGER                = 400,
    RBAC_OPEN_EMPLOYEE_MANAGER              = 401,
    RBAC_CREATE_TICKET                      = 402,
    RBAC_SHOW_TICKETS                       = 403,
    RBAC_MODIFY_TICKET                      = 404,
    RBAC_OPEN_TV                            = 405,
    RBAC_OPEN_MACHINE_AND_LINES             = 406,
    RBAC_SEARCH_OPEN_TICKETS                = 407,

    // Ticket details page
    RBAC_CHANGE_TICKET_TITLE_DESCRIPTION    = 408,
    RBAC_CHANGE_TICKET_PRIORITY             = 409,
    RBAC_CHANGE_TICKET_STATUS               = 410,
    RBAC_ASSIGN_UNASSIGN_EMPLOYEE           = 411,
    RBAC_UPLOAD_FILES                       = 412,
    RBAC_ADD_TICKET_COMMENT                 = 413,
    RBAC_ADD_SPARE_PART                     = 414,
    RBAC_REMOVE_SPARE_PART                  = 415,
    RBAC_CREATE_REPORT                      = 416,
    RBAC_MODIFY_REPORT                      = 417,
    RBAC_CLOSE_TICKET                       = 418,

};

enum class PermissionGroups : int
{
	PERMISSION_GROUP_OBSERVER           = 1,
	PERMISSION_GROUP_OPERATOR           = 2,
	PERMISSION_GROUP_COORDINATOR        = 3,
	PERMISSION_GROUP_MANAGER            = 4,
	PERMISSION_GROUP_ADMIN				= 5,
};

inline std::string PermissionToString(Permission permission)
{
	switch (permission)
	{
        case Permission::ROLE_OBSERVER: return "Observer";                    
		case Permission::ROLE_OPERATOR: return "Operator";
        case Permission::ROLE_COORDINATOR:return "Coordinator";
        case Permission::ROLE_MANAGER: return "Manager";
		case Permission::ROLE_ADMIN: return "Admin";
		case Permission::ROLE_MAX_ROLE: return "Max Role";

			// Permissions
		default: return "Unknown";
	}
}
