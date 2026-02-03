#include "DatabaseManager.h"

namespace database
{

DatabaseManager& DatabaseManager::Instance()
{
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::Configure(const PoolConfig& config)
{
    config_ = config;
    pool_.Configure(config);
}

std::shared_ptr<DatabaseConnection> DatabaseManager::GetConnection(ConnectionType type, bool preferReplica)
{
    return pool_.Acquire(type, preferReplica && config_.useReplicaForReads);
}

void DatabaseManager::ReturnConnection(const std::shared_ptr<DatabaseConnection>& connection)
{
    pool_.Release(connection);
}

CancellationToken DatabaseManager::ExecuteAsync(ConnectionType type, std::function<void(std::shared_ptr<DatabaseConnection>)> task, bool preferReplica)
{
    return pool_.SubmitAsync(type, std::move(task), preferReplica && config_.useReplicaForReads);
}

DiagnosticsSnapshot DatabaseManager::GetDiagnostics() const
{
    return pool_.GetDiagnostics();
}

void DatabaseManager::Shutdown()
{
    pool_.Shutdown();
}

} // namespace database
