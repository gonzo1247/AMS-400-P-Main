#pragma once
#include "DatabaseDefines.h"

class CreateTicketManager
{
public:
	CreateTicketManager();
    ~CreateTicketManager() = default;

    bool SaveTicket(const TicketInformation& ticketInfo, const TicketAssignmentInformation& assignmentInfo);


private:
	bool CreateNewTicket(const TicketInformation& ticketInfo);
    bool CreateNewTicketAssignment(const TicketAssignmentInformation& assignmentInfo);

	std::uint32_t _lastTicketID;
};

