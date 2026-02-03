
#include "ConnectionPool.h"

#include <chrono>
#include <stdexcept>

#include "MySQLPreparedStatements.h"
#include "PreparedStatementRegistry.h"

namespace database
{

namespace
{

std::shared_ptr<DatabaseConnection> CreateConnection(const MySQLSettings& settings, ConnectionType type, bool replica)
{
    auto connection = std::make_shared<DatabaseConnection>(settings, type, replica);
    if (!connection->Connect())
        throw std::runtime_error("Failed to establish database connection");
    return connection;
}

} // namespace

ConnectionPool& ConnectionPool::Instance()
{
    static ConnectionPool instance;
    return instance;
}

std::shared_ptr<DatabaseConnection> ConnectionPool::GetConnection(ConnectionType type, bool preferReplica)
{
    auto& pool = Instance();
    return pool.Acquire(type, preferReplica && pool.config_.useReplicaForReads);
}

void ConnectionPool::ReturnConnection(const std::shared_ptr<DatabaseConnection>& connection)
{
    Instance().Release(connection);
}

ConnectionPool::ConnectionPool() : maintenanceRunning_(false)
{
    RegisterPreparedStatements();
}

ConnectionPool::~ConnectionPool()
{
    StopMaintenance();
}

void ConnectionPool::Configure(const PoolConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    syncConnections_.clear();
    asyncConnections_.clear();
    replicaSyncConnections_.clear();
    replicaAsyncConnections_.clear();

    while (!availableSync_.empty())
        availableSync_.pop();
    while (!availableAsync_.empty())
        availableAsync_.pop();
    while (!availableReplicaSync_.empty())
        availableReplicaSync_.pop();
    while (!availableReplicaAsync_.empty())
        availableReplicaAsync_.pop();

    if (config.primary.isActive)
    {
        InitializePool(ConnectionType::Sync, config.syncLimits, config.primary, false);
        InitializePool(ConnectionType::Async, config.asyncLimits, config.primary, false);
    }
    else
    {
        LOG_WARNING("Primary database configuration is inactive; skipping pool initialization.");
    }

    if (config.useReplicaForReads && config.replicaConfig.enabled && config.replica.isActive)
    {
        InitializePool(ConnectionType::Sync, config.syncLimits, config.replica, true);
        InitializePool(ConnectionType::Async, config.asyncLimits, config.replica, true);
    }

    maintenanceRunning_.store(false);
    if (maintenanceThread_.joinable())
        maintenanceThread_.join();
    maintenanceRunning_.store(true);
    maintenanceThread_ = std::thread(&ConnectionPool::MaintenanceLoop, this);
}

std::shared_ptr<DatabaseConnection> ConnectionPool::Acquire(ConnectionType type, bool preferReplica)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto& primaryQueue = (type == ConnectionType::Sync) ? availableSync_ : availableAsync_;
    auto& replicaQueue = (type == ConnectionType::Sync) ? availableReplicaSync_ : availableReplicaAsync_;

    while (true)
    {
        if (preferReplica && !replicaQueue.empty())
        {
            auto connection = replicaQueue.front();
            replicaQueue.pop();
            return connection;
        }

        if (!primaryQueue.empty())
        {
            auto connection = primaryQueue.front();
            primaryQueue.pop();
            return connection;
        }

        if (config_.useReplicaForReads && !replicaQueue.empty())
        {
            auto connection = replicaQueue.front();
            replicaQueue.pop();
            return connection;
        }

        cv_.wait(lock);
    }
}

void ConnectionPool::Release(const std::shared_ptr<DatabaseConnection>& connection)
{
    if (!connection)
        return;

    std::scoped_lock lock(mutex_);

    if (connection->IsReplica())
    {
        auto& queue = (connection->GetConnectionType() == ConnectionType::Sync) ? availableReplicaSync_ : availableReplicaAsync_;
        queue.push(connection);
    }
    else
    {
        auto& queue = (connection->GetConnectionType() == ConnectionType::Sync) ? availableSync_ : availableAsync_;
        queue.push(connection);
    }

    cv_.notify_one();
}

CancellationToken ConnectionPool::SubmitAsync(ConnectionType type, std::function<void(std::shared_ptr<DatabaseConnection>)> task, bool preferReplica)
{
    auto maxDepth = (type == ConnectionType::Sync) ? config_.syncLimits.maxQueueDepth : config_.asyncLimits.maxQueueDepth;
    return asyncExecutor_.Submit(
        [this, task = std::move(task), preferReplica, type]() {
            auto connection = Acquire(type, preferReplica && config_.useReplicaForReads);
            try
            {
                task(connection);
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR(std::string("Async query failed: ") + ex.what());
            }
            Release(connection);
        },
        maxDepth);
}

void ConnectionPool::StartMaintenance()
{
    bool expected = false;
    if (maintenanceRunning_.compare_exchange_strong(expected, true))
    {
        maintenanceThread_ = std::thread(&ConnectionPool::MaintenanceLoop, this);
    }
}

void ConnectionPool::StopMaintenance()
{
    bool expected = true;
    if (maintenanceRunning_.compare_exchange_strong(expected, false))
    {
        maintenanceCv_.notify_all();
        if (maintenanceThread_.joinable())
            maintenanceThread_.join();
    }
    asyncExecutor_.Stop();
    cv_.notify_all();
}

void ConnectionPool::Shutdown()
{
    StopMaintenance();

    std::scoped_lock lock(mutex_);

    auto disconnectAll = [](std::vector<std::shared_ptr<DatabaseConnection>>& connections) {
        for (auto& connection : connections)
        {
            if (connection)
                connection->Disconnect();
        }
        connections.clear();
    };

    disconnectAll(syncConnections_);
    disconnectAll(asyncConnections_);
    disconnectAll(replicaSyncConnections_);
    disconnectAll(replicaAsyncConnections_);

    while (!availableSync_.empty())
        availableSync_.pop();
    while (!availableAsync_.empty())
        availableAsync_.pop();
    while (!availableReplicaSync_.empty())
        availableReplicaSync_.pop();
    while (!availableReplicaAsync_.empty())
        availableReplicaAsync_.pop();

    cv_.notify_all();
}

DiagnosticsSnapshot ConnectionPool::GetDiagnostics() const
{
    DiagnosticsSnapshot snapshot;
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot.syncPoolSize = syncConnections_.size();
    snapshot.asyncPoolSize = asyncConnections_.size();
    snapshot.replicaSyncPoolSize = replicaSyncConnections_.size();
    snapshot.replicaAsyncPoolSize = replicaAsyncConnections_.size();
    snapshot.syncAvailable = availableSync_.size();
    snapshot.asyncAvailable = availableAsync_.size();
    snapshot.replicaSyncAvailable = availableReplicaSync_.size();
    snapshot.replicaAsyncAvailable = availableReplicaAsync_.size();
    snapshot.queuedJobs = asyncExecutor_.QueueSize();
    snapshot.registeredStatements = PreparedStatementRegistry::Instance().GetAll().size();
    return snapshot;
}

void ConnectionPool::InitializePool(ConnectionType type, const PoolLimits& limits, const MySQLSettings& settings, bool replica)
{
    auto& connections = replica ? ((type == ConnectionType::Sync) ? replicaSyncConnections_ : replicaAsyncConnections_)
                                 : ((type == ConnectionType::Sync) ? syncConnections_ : asyncConnections_);
    auto& queue = replica ? ((type == ConnectionType::Sync) ? availableReplicaSync_ : availableReplicaAsync_)
                          : ((type == ConnectionType::Sync) ? availableSync_ : availableAsync_);

    for (std::size_t i = 0; i < limits.minSize; ++i)
    {
        try
        {
            auto connection = CreateConnection(settings, type, replica);
            connections.push_back(connection);
            queue.push(connection);
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR(std::string("Failed to initialize connection: ") + ex.what());
        }
    }
}

void ConnectionPool::MaintenanceLoop()
{
    while (maintenanceRunning_.load())
    {
        std::unique_lock<std::mutex> maintenanceLock(maintenanceMutex_);
        maintenanceCv_.wait_for(maintenanceLock,
                                std::chrono::seconds(config_.maintenance.pingIntervalSeconds),
                                [this]() { return !maintenanceRunning_.load(); });
        if (!maintenanceRunning_.load())
            break;

        std::unique_lock<std::mutex> lock(mutex_);
        auto checkConnections = [&](std::vector<std::shared_ptr<DatabaseConnection>>& connections, std::queue<std::shared_ptr<DatabaseConnection>>& availableQueue, bool replica) {
            for (auto& connection : connections)
            {
                if (!EnsureConnected(connection))
                    continue;

                if (!connection->Ping())
                {
                    LOG_WARNING("Ping failed, reconnecting...");
                    connection->Disconnect();
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::seconds(config_.maintenance.reconnectDelaySeconds));
                    lock.lock();
                    connection->Connect();
                }

                if (replica && config_.replicaConfig.enabled && !config_.replicaConfig.lagQuery.empty())
                {
                    try
                    {
                        lock.unlock();
                        auto lagResult = connection->ExecuteSelect(config_.replicaConfig.lagQuery);
                        lock.lock();
                        if (lagResult.IsValid() && lagResult.GetFieldCount() > 0)
                        {
                            if (Field* fields = lagResult.Fetch())
                            {
                                const Field& lagField = fields[0];
                                if (lagField.IsNull())
                                    continue;

                                auto lagString = lagField.ToString();
                                std::uint64_t lagSeconds = 0;
                                if (!lagString.empty())
                                    lagSeconds = static_cast<std::uint64_t>(std::stoull(lagString));
                                if (lagSeconds > config_.replicaConfig.maxAllowedLagSeconds)
                                {
                                    LOG_WARNING("Replica lag exceeds configured threshold: " + std::to_string(lagSeconds) + "s");
                                }
                            }
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        if (!lock.owns_lock())
                            lock.lock();
                        LOG_ERROR(std::string("Replica lag check failed: ") + ex.what());
                    }
                }
            }

            std::queue<std::shared_ptr<DatabaseConnection>> refreshed;
            while (!availableQueue.empty())
            {
                auto conn = availableQueue.front();
                availableQueue.pop();
                if (conn->IsConnected())
                    refreshed.push(conn);
            }
            std::swap(availableQueue, refreshed);
        };

        checkConnections(syncConnections_, availableSync_, false);
        checkConnections(asyncConnections_, availableAsync_, false);
        checkConnections(replicaSyncConnections_, availableReplicaSync_, true);
        checkConnections(replicaAsyncConnections_, availableReplicaAsync_, true);
    }
}

bool ConnectionPool::EnsureConnected(const std::shared_ptr<DatabaseConnection>& connection)
{
    if (!connection->IsConnected())
    {
        LOG_WARNING("Reconnecting database connection...");
        return connection->Connect();
    }
    return true;
}

} // namespace database
