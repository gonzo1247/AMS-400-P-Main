#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "AsyncExecutor.h"
#include "DatabaseConnection.h"
#include "PreparedStatement.h"

namespace database
{

struct DiagnosticsSnapshot
{
    std::size_t syncPoolSize = 0;
    std::size_t asyncPoolSize = 0;
    std::size_t replicaSyncPoolSize = 0;
    std::size_t replicaAsyncPoolSize = 0;
    std::size_t syncAvailable = 0;
    std::size_t asyncAvailable = 0;
    std::size_t replicaSyncAvailable = 0;
    std::size_t replicaAsyncAvailable = 0;
    std::size_t queuedJobs = 0;
    std::size_t registeredStatements = 0;
};

class ConnectionPool
{
public:
    ConnectionPool();
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    static ConnectionPool& Instance();

    void Configure(const PoolConfig& config);

    static std::shared_ptr<DatabaseConnection> GetConnection(ConnectionType type, bool preferReplica = false);
    static void ReturnConnection(const std::shared_ptr<DatabaseConnection>& connection);

    std::shared_ptr<DatabaseConnection> Acquire(ConnectionType type, bool preferReplica = false);
    void Release(const std::shared_ptr<DatabaseConnection>& connection);

    CancellationToken SubmitAsync(ConnectionType type, std::function<void(std::shared_ptr<DatabaseConnection>)> task,
                                  bool preferReplica = false);

    void StartMaintenance();
    void StopMaintenance();

    DiagnosticsSnapshot GetDiagnostics() const;

private:

    void InitializePool(ConnectionType type, const PoolLimits& limits, const MySQLSettings& settings, bool replica);
    void MaintenanceLoop();
    bool EnsureConnected(const std::shared_ptr<DatabaseConnection>& connection);

    PoolConfig config_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::vector<std::shared_ptr<DatabaseConnection>> syncConnections_;
    std::vector<std::shared_ptr<DatabaseConnection>> asyncConnections_;
    std::vector<std::shared_ptr<DatabaseConnection>> replicaSyncConnections_;
    std::vector<std::shared_ptr<DatabaseConnection>> replicaAsyncConnections_;
    std::queue<std::shared_ptr<DatabaseConnection>> availableSync_;
    std::queue<std::shared_ptr<DatabaseConnection>> availableAsync_;
    std::queue<std::shared_ptr<DatabaseConnection>> availableReplicaSync_;
    std::queue<std::shared_ptr<DatabaseConnection>> availableReplicaAsync_;

    AsyncExecutor asyncExecutor_;

    std::atomic<bool> maintenanceRunning_;
    std::thread maintenanceThread_;
};

} // namespace database

