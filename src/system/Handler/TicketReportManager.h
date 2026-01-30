#pragma once
#include "DatabaseDefines.h"

class TicketReportManager
{
public:
    explicit TicketReportManager();

    TicketReportData LoadTicketReport(std::uint64_t ticketID);
    bool SaveNewReport(const TicketReportData& data);
    bool SaveReportUpdate(const TicketReportData& data);
};
