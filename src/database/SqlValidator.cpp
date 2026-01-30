#include "SqlValidator.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include "ConnectionGuard.h"
#include "DatabaseConnection.h"
#include "Implementation/AMSDatabase.h"
#include "Implementation/IMSDatabase.h"
#include "PreparedStatementRegistry.h"

namespace database
{
namespace
{
    enum class DatabaseKind : std::uint8_t
    {
        IMS,
        AMS
    };

    bool SupportsConnectionType(const StatementMetadata& metadata, ConnectionType connectionType)
    {
        switch (metadata.connectionType)
        {
            case StatementConnectionType::Both:
                return true;
            case StatementConnectionType::Sync:
                return connectionType == ConnectionType::Sync;
            case StatementConnectionType::Async:
                return connectionType == ConnectionType::Async;
        }

        return false;
    }

    bool IsStatementForDatabase(StatementName name, DatabaseKind kind)
    {
        const auto id = static_cast<std::uint32_t>(name);
        switch (kind)
        {
            case DatabaseKind::IMS:
                return id > static_cast<std::uint32_t>(StatementName::NONE) &&
                       id < static_cast<std::uint32_t>(Implementation::IMSPreparedStatement::MAX_VALUE);
            case DatabaseKind::AMS:
                return id >= static_cast<std::uint32_t>(Implementation::IMSPreparedStatement::MAX_VALUE) &&
                       id < static_cast<std::uint32_t>(Implementation::AMSPreparedStatement::MAX_VALUE);
        }

        return false;
    }

    std::string StatementLabel(const StatementMetadata& metadata)
    {
        if (!metadata.alias.empty())
            return metadata.alias;
        return std::to_string(static_cast<std::uint32_t>(metadata.name));
    }

    template <typename Guard>
    void ValidateForConnectionType(DatabaseKind kind, std::string_view dbName, ConnectionType connectionType)
    {
        const auto start = std::chrono::steady_clock::now();
        Guard guard(connectionType);
        auto connection = guard.Get();

        if (!connection)
        {
            LOG_DEBUG("No {} {} connection available for validation", dbName, connectionType == ConnectionType::Sync ? "sync" : "async");
            return;
        }

        const auto allStatements = PreparedStatementRegistry::Instance().GetAll();
        int successCount = 0;
        int failCount = 0;

        for (const auto& metadata : allStatements)
        {
            if (!IsStatementForDatabase(metadata.name, kind))
                continue;
            if (!SupportsConnectionType(metadata, connectionType))
                continue;

            std::string error;
            if (!connection->TryPrepareStatement(metadata.name, error))
            {
                LOG_DEBUG("❌ [{} {}] Statement {} failed:\n{}", dbName, connectionType == ConnectionType::Sync ? "sync" : "async", StatementLabel(metadata), error);
                ++failCount;
            }
            else
            {
                LOG_DEBUG("✅ [{} {}] Statement {} OK", dbName, connectionType == ConnectionType::Sync ? "sync" : "async", StatementLabel(metadata));
                ++successCount;
            }
        }

        const auto end = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        LOG_DEBUG("🧾 [{} {}] Validation complete. Success: {}, Failed: {} | Duration: {} ms", dbName, connectionType == ConnectionType::Sync ? "sync" : "async", successCount, failCount, duration);
    }

    template <typename Guard>
    void ValidateDatabaseStatements(DatabaseKind kind, std::string_view dbName)
    {
        LOG_DEBUG("Validating prepared SQL statements for {}...", dbName);
        ValidateForConnectionType<Guard>(kind, dbName, ConnectionType::Sync);
        ValidateForConnectionType<Guard>(kind, dbName, ConnectionType::Async);
    }
} // namespace

void SqlValidator::ValidateAllStatements()
{
    ValidateDatabaseStatements<ConnectionGuardIMS>(DatabaseKind::IMS, "IMS");
    ValidateDatabaseStatements<ConnectionGuardAMS>(DatabaseKind::AMS, "AMS");
}

} // namespace database
