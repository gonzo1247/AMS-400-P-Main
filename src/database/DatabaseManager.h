#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ConnectionPool.h"
#include "DatabaseConnection.h"
#include "PreparedStatementRegistry.h"

namespace database
{

namespace detail
{
    template <typename Tag>
    class NamedDatabase;
}

class DatabaseManager
{
public:
    static DatabaseManager& Instance();

    void Configure(const PoolConfig& config);

    std::shared_ptr<DatabaseConnection> GetConnection(ConnectionType type, bool preferReplica = false);
    void ReturnConnection(const std::shared_ptr<DatabaseConnection>& connection);
    CancellationToken ExecuteAsync(ConnectionType type, std::function<void(std::shared_ptr<DatabaseConnection>)> task, bool preferReplica = false);

    DiagnosticsSnapshot GetDiagnostics() const;

private:
    DatabaseManager() = default;

    template <typename Tag>
    friend class detail::NamedDatabase;

    PoolConfig config_;
    ConnectionPool pool_;
};

} // namespace database

