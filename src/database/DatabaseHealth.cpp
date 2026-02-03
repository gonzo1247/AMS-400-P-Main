#include "DatabaseHealth.h"

#include "SettingsManager.h"
#include "DatabaseConnection.h"

namespace database
{
    namespace
    {

        bool CheckDatabaseReachable(const MySQLSettings& settings)
        {
            if (!settings.isActive)
                return false;

            DatabaseConnection connection(settings, ConnectionType::Sync, false);
            if (!connection.Connect())
                return false;

            return connection.Ping();
        }

    }  // namespace

    bool AreDatabasesReachable(const MySQLSettings& imsSettings, const MySQLSettings& amsSettings)
    {
        return CheckDatabaseReachable(imsSettings) && CheckDatabaseReachable(amsSettings);
    }

}  // namespace database
