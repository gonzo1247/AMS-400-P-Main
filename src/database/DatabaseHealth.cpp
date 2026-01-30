#include "DatabaseHealth.h"

#include <string>

namespace database
{
    namespace
    {

        template <typename Guard>
        bool CheckDatabaseReachable(std::string_view label)
        {
            Guard guard(ConnectionType::Sync, false, std::string(label) + "-health-check");
            if (!guard)
                return false;

            if (!guard->IsConnected() && !guard->Connect())
                return false;

            return guard->Ping();
        }

    }  // namespace

    bool ArePrimaryDatabasesReachable()
    {
        return CheckDatabaseReachable<ConnectionGuardIMS>("IMS") && CheckDatabaseReachable<ConnectionGuardAMS>("AMS");
    }

}  // namespace database
