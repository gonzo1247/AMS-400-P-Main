#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <mariadb/conncpp.hpp>

#include "DatabaseTypes.h"
#include "PreparedStatement.h"
#include "PreparedStatementNames.h"
#include "QueryResult.h"
#include "SettingsManager.h"

namespace database
{

class PreparedStatementRegistry;
class SshTunnel;

class DatabaseConnection
{
public:
    DatabaseConnection(const MySQLSettings& settings, ConnectionType type, bool replica = false);
    ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    bool Connect();
    bool IsConnected() const;
    void Disconnect();

    QueryResult ExecuteSelect(const std::string& query);
    bool ExecuteInsert(const std::string& query);
    bool ExecuteUpdate(const std::string& query);
    bool ExecuteDelete(const std::string& query);

    QueryResult ExecutePreparedSelect(PreparedStatement& statement);
    QueryResult ExecuteAdhocPreparedSelect(const std::string& query, const std::vector<std::string>& params);
    bool ExecutePreparedInsert(PreparedStatement& statement);
    bool ExecutePreparedUpdate(PreparedStatement& statement);
    std::uint64_t ExecutePreparedDelete(PreparedStatement& statement);
    std::uint64_t ExecutePreparedModification(PreparedStatement& statement);

    std::uint64_t GetAffectedRows() const { return lastAffectedRows_; }

    PreparedStatementPtr GetPreparedStatement(StatementName name);
    PreparedStatementPtr GetPreparedStatement(PreparedStatementIndex name);
    PreparedStatementPtr GetPreparedStatement(const std::string& alias);
    PreparedStatementPtr GetStatementRaw(const std::string& query);

    template <typename StatementEnum>
    PreparedStatementPtr GetPreparedStatement(StatementEnum statement)
    {
        return GetPreparedStatement(ToStatementName(statement));
    }

    PreparedStatementSharedPtr GetSharedPreparedStatement(StatementName name);
    PreparedStatementSharedPtr GetSharedPreparedStatement(PreparedStatementIndex name);
    PreparedStatementSharedPtr GetSharedPreparedStatement(const std::string& alias);

    template <typename StatementEnum>
    PreparedStatementSharedPtr GetSharedPreparedStatement(StatementEnum statement)
    {
        return GetSharedPreparedStatement(ToStatementName(statement));
    }

    sql::PreparedStatement* GetRawPreparedStatement(StatementName name);
    sql::PreparedStatement* GetRawPreparedStatement(PreparedStatementIndex name);
    sql::PreparedStatement* GetRawPreparedStatement(const std::string& alias);

    template <typename StatementEnum>
    sql::PreparedStatement* GetRawPreparedStatement(StatementEnum statement)
    {
        return GetRawPreparedStatement(ToStatementName(statement));
    }

    sql::Connection* GetRawConnection() { return connection_.get(); }

    bool TryPrepareStatement(StatementName name, std::string& error);

    bool Ping();
    std::uint64_t GetLastInsertId();

    void BeginTransaction();
    void Commit();
    void Rollback();

    const MySQLSettings& GetSettings() const { return settings_; }
    ConnectionType GetConnectionType() const { return type_; }
    bool IsReplica() const { return replica_; }

private:
    std::unique_ptr<sql::Statement> CreateStatement();
    StatementMetadata LookupMetadata(StatementName name);
    StatementMetadata LookupMetadata(PreparedStatementIndex name);
    StatementMetadata LookupMetadata(const std::string& alias);

    PreparedStatementSharedPtr GetSharedInternal(const StatementMetadata& metadata);

    MySQLSettings settings_;
    ConnectionType type_;
    bool replica_;
    std::unique_ptr<sql::Connection> connection_;

    std::uint64_t lastAffectedRows_ = 0;

    mutable std::mutex preparedMutex_;
    std::unordered_map<StatementName, PreparedStatementSharedPtr> sharedByName_;
    std::unordered_map<std::string, PreparedStatementSharedPtr> sharedByAlias_;
    std::vector<PreparedStatementPtr> rawCache_;

    std::unique_ptr<SshTunnel> sshTunnel_;
};

} // namespace database

