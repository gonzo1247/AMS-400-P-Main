#include "DatabaseConnection.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "PreparedStatementRegistry.h"
#include "ssh/SshTunnel.h"

namespace database
{

namespace
{
    static sql::SQLString BuildJdbcUrl(const MySQLSettings& s)
    {
        // Example: jdbc:mariadb://host:port/database?sslMode=REQUIRED&sslVerify=true
        std::string url;
        url.reserve(256);

        url += "jdbc:mariadb://";
        url += s.hostname;
        url += ":";
        url += s.port;
        if (!s.database.empty())
        {
            url += "/";
            url += s.database;
        }

        bool first = true;
        auto addParam = [&](std::string key, std::string val)
        {
            url += (first ? "?" : "&");
            first = false;
            url += std::move(key);
            url += "=";
            url += std::move(val);
        };

        if (s.tls.enabled)
        {
            addParam("sslMode", "REQUIRED");
            addParam("sslVerify", s.tls.verifyServerCert ? "true" : "false");

            if (!s.tls.caFile.empty())
                addParam("sslCA", s.tls.caFile);
            if (!s.tls.certFile.empty())
                addParam("sslCert", s.tls.certFile);
            if (!s.tls.keyFile.empty())
                addParam("sslKey", s.tls.keyFile);
            if (!s.tls.cipherList.empty())
                addParam("sslCipher", s.tls.cipherList);
        }

        return sql::SQLString(url);
    }

} // namespace

DatabaseConnection::DatabaseConnection(const MySQLSettings& settings, ConnectionType type, bool replica)
    : settings_(settings), type_(type), replica_(replica)
{
}

DatabaseConnection::~DatabaseConnection()
{
    Disconnect();
}

bool DatabaseConnection::Connect()
{
    try
    {
        MySQLSettings effectiveSettings = settings_;
        if (settings_.ssh.enabled)
        {
            sshTunnel_ = std::make_unique<SshTunnel>(settings_.ssh, settings_.hostname, Util::ConvertStringToUint16(settings_.port));
            if (!sshTunnel_->Start())
            {
                LOG_ERROR("Failed to establish SSH tunnel");
                sshTunnel_.reset();
                return false;
            }

            effectiveSettings.hostname = sshTunnel_->GetLocalHost();
            effectiveSettings.port = sshTunnel_->GetLocalPort();
        }
        else
        {
            sshTunnel_.reset();
        }

        sql::Driver* driver = sql::mariadb::get_driver_instance();
        const auto url = BuildJdbcUrl(effectiveSettings);
        connection_.reset(driver->connect(url, effectiveSettings.username, effectiveSettings.password));

        if (settings_.ssh.enabled)
            LOG_SQL("Connected to database via SSH tunnel: " + settings_.hostname);
        else
            LOG_SQL("Connected to database: " + settings_.hostname);

        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("Failed to connect: ") + ex.what());
        connection_.reset();
        if (sshTunnel_)
        {
            sshTunnel_->Stop();
            sshTunnel_.reset();
        }
        return false;
    }
}

bool DatabaseConnection::IsConnected() const
{
    return connection_ != nullptr;
}

void DatabaseConnection::Disconnect()
{
    if (connection_)
    {
        connection_->close();
        connection_.reset();
    }

    std::lock_guard<std::mutex> lock(preparedMutex_);
    sharedByName_.clear();
    sharedByAlias_.clear();
    rawCache_.clear();

    if (sshTunnel_)
    {
        sshTunnel_->Stop();
        sshTunnel_.reset();
    }
}

QueryResult DatabaseConnection::ExecuteSelect(const std::string& query)
{
    auto statement = CreateStatement();
    auto result = std::unique_ptr<sql::ResultSet>(statement->executeQuery(query));
    lastAffectedRows_ = 0;
    return QueryResult(std::move(result));
}

bool DatabaseConnection::ExecuteInsert(const std::string& query)
{
    try
    {     
        auto statement = CreateStatement();
        statement->execute(query);
        lastAffectedRows_ = statement->getUpdateCount();
        return true;
    }
    catch (sql::SQLException& e)
    {
        LOG_ERROR("SQL error during insert: {}", e.getMessage());
    }
    catch (...)
    {
        LOG_ERROR("Failed to execute insert: unknown error");
    }

    return false;
}

bool DatabaseConnection::ExecuteUpdate(const std::string& query)
{
    try
    {
        auto statement = CreateStatement();
        statement->execute(query);
        lastAffectedRows_ = statement->getUpdateCount();
        return true;
    }
    catch (sql::SQLException& e)
    {
        LOG_ERROR("SQL error during insert: {}", e.getMessage());
    }
    catch (...)
    {
        LOG_ERROR("Failed to execute insert: unknown error");
    }

    return false;
}

bool DatabaseConnection::ExecuteDelete(const std::string& query)
{
    try
    {
        auto statement = CreateStatement();
        statement->execute(query);
        lastAffectedRows_ = statement->getUpdateCount();
        return true;
    }
    catch (sql::SQLException& e)
    {
        LOG_ERROR("SQL error during insert: {}", e.getMessage());
    }
    catch (...)
    {
        LOG_ERROR("Failed to execute insert: unknown error");
    }

    return false;
}

QueryResult DatabaseConnection::ExecutePreparedSelect(PreparedStatement& statement)
{
    try
    {
        auto result = std::unique_ptr<sql::ResultSet>(statement.GetRaw()->executeQuery());
        lastAffectedRows_ = 0;
        return QueryResult(std::move(result));
    }
    catch (sql::SQLException& ex)
    {
        const auto& md = statement.GetMetadata();
        LOG_SQL(
            "SQL executeQuery failed\n"
            "Name: {}\n"
            "Alias: {}\n"
            "Query: {}\n"
            "Error: {}\n"
            "Code: {}\n"
            "State: {}",
            md.name, md.alias, md.query, ex.what(), ex.getErrorCode(), ex.getSQLState());
        throw;
    }
}

QueryResult DatabaseConnection::ExecuteAdhocPreparedSelect(const std::string& query, const std::vector<std::string>& params)
{
    try
    {
        if (!connection_)
        {
            lastAffectedRows_ = 0;
            LOG_ERROR("Failed to execute adhoc prepared select: database connection is not established");
            return QueryResult(std::unique_ptr<sql::ResultSet>{});
        }

        auto statement = std::unique_ptr<sql::PreparedStatement>(connection_->prepareStatement(query));
        for (std::size_t i = 0; i < params.size(); ++i)
            statement->setString(static_cast<int>(i + 1), params[i]);

        auto result = std::unique_ptr<sql::ResultSet>(statement->executeQuery());
        lastAffectedRows_ = 0;
        return QueryResult(std::move(result), std::move(statement));
    }
    catch (sql::SQLException& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("SQL error during adhoc prepared select: {}", ex.getMessage());
    }
    catch (const std::exception& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR(std::string("Failed to execute adhoc prepared select: ") + ex.what());
    }
    catch (...)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("Failed to execute adhoc prepared select: unknown error");
    }

    return QueryResult(std::unique_ptr<sql::ResultSet>{});
}

bool DatabaseConnection::ExecutePreparedInsert(PreparedStatement& statement)
{
    try
    {
        auto* raw = statement.GetRaw();
        if (raw == nullptr)
        {
            lastAffectedRows_ = 0;
            LOG_ERROR("Failed to execute prepared insert: invalid statement handle");
            return false;
        }

        raw->execute();
        lastAffectedRows_ = raw->getUpdateCount();
        return true;
    }
    catch (const std::exception& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR(std::string("Failed to execute prepared insert: ") + ex.what());
    }
    catch (...)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("Failed to execute prepared insert: unknown error");
    }

    return false;
}

bool DatabaseConnection::ExecutePreparedUpdate(PreparedStatement& statement)
{
    try
    {
        auto* raw = statement.GetRaw();
        if (raw == nullptr)
        {
            lastAffectedRows_ = 0;
            LOG_ERROR("Failed to execute prepared update: invalid statement handle");
            return false;
        }

        raw->execute();
        lastAffectedRows_ = raw->getUpdateCount();
        return true;
    }
    catch (const std::exception& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR(std::string("Failed to execute prepared update: ") + ex.what());
    }
    catch (...)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("Failed to execute prepared update: unknown error");
    }

    return false;
}

std::uint64_t DatabaseConnection::ExecutePreparedDelete(PreparedStatement& statement)
{
    try
    {
        auto* raw = statement.GetRaw();
        if (raw == nullptr)
        {
            lastAffectedRows_ = 0;
            LOG_ERROR("Failed to execute prepared delete: invalid statement handle");
            return lastAffectedRows_;
        }

        raw->execute();
        lastAffectedRows_ = raw->getUpdateCount();
    }
    catch (const std::exception& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR(std::string("Failed to execute prepared delete: ") + ex.what());
    }
    catch (...)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("Failed to execute prepared delete: unknown error");
    }

    return lastAffectedRows_;
}

std::uint64_t DatabaseConnection::ExecutePreparedModification(PreparedStatement& statement)
{
    try
    {
        auto* raw = statement.GetRaw();
        if (raw == nullptr)
        {
            lastAffectedRows_ = 0;
            LOG_ERROR("Failed to execute prepared modification: invalid statement handle");
            return lastAffectedRows_;
        }

        raw->execute();
        lastAffectedRows_ = raw->getUpdateCount();
    }
    catch (const std::exception& ex)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR(std::string("Failed to execute prepared modification: ") + ex.what());
    }
    catch (...)
    {
        lastAffectedRows_ = 0;
        LOG_ERROR("Failed to execute prepared modification: unknown error");
    }
    return lastAffectedRows_;
}

PreparedStatementPtr DatabaseConnection::GetPreparedStatement(StatementName name)
{
    auto metadata = LookupMetadata(name);
    return MakePreparedStatement(connection_.get(), metadata);
}

PreparedStatementPtr DatabaseConnection::GetPreparedStatement(PreparedStatementIndex name)
{
    auto metadata = LookupMetadata(name);
    return MakePreparedStatement(connection_.get(), metadata);
}

PreparedStatementPtr DatabaseConnection::GetPreparedStatement(const std::string& alias)
{
    auto metadata = LookupMetadata(alias);
    return MakePreparedStatement(connection_.get(), metadata);
}

PreparedStatementPtr DatabaseConnection::GetStatementRaw(const std::string& query)
{
    StatementMetadata metadata;
    metadata.name = StatementName::NONE;
    metadata.alias = "<raw-sql>";
    metadata.query = query;
    metadata.connectionType = StatementConnectionType::Sync;

    return MakePreparedStatement(connection_.get(), metadata);
}

PreparedStatementSharedPtr DatabaseConnection::GetSharedPreparedStatement(StatementName name)
{
    auto metadata = LookupMetadata(name);
    return GetSharedInternal(metadata);
}

PreparedStatementSharedPtr DatabaseConnection::GetSharedPreparedStatement(PreparedStatementIndex name)
{
    auto metadata = LookupMetadata(name);
    return GetSharedInternal(metadata);
}

PreparedStatementSharedPtr DatabaseConnection::GetSharedPreparedStatement(const std::string& alias)
{
    auto metadata = LookupMetadata(alias);
    return GetSharedInternal(metadata);
}

sql::PreparedStatement* DatabaseConnection::GetRawPreparedStatement(StatementName name)
{
    auto metadata = LookupMetadata(name);
    auto prepared = MakePreparedStatement(connection_.get(), metadata);
    auto raw = prepared->GetRaw();
    {
        std::scoped_lock lock(preparedMutex_);
        rawCache_.push_back(std::move(prepared));
    }
    return raw;
}

sql::PreparedStatement* DatabaseConnection::GetRawPreparedStatement(PreparedStatementIndex name)
{
    auto metadata = LookupMetadata(name);
    auto prepared = MakePreparedStatement(connection_.get(), metadata);
    auto raw = prepared->GetRaw();
    {
        std::scoped_lock lock(preparedMutex_);
        rawCache_.push_back(std::move(prepared));
    }
    return raw;
}

sql::PreparedStatement* DatabaseConnection::GetRawPreparedStatement(const std::string& alias)
{
    auto metadata = LookupMetadata(alias);
    auto prepared = MakePreparedStatement(connection_.get(), metadata);
    auto raw = prepared->GetRaw();
    {
        std::scoped_lock lock(preparedMutex_);
        rawCache_.push_back(std::move(prepared));
    }
    return raw;
}

bool DatabaseConnection::TryPrepareStatement(StatementName name, std::string& error)
{
    try
    {
        auto metadata = LookupMetadata(name);
        (void)MakePreparedStatement(connection_.get(), metadata);

        // Run a lightweight EXPLAIN against the statement to ensure referenced tables
        // actually exist. Some drivers only validate syntax during prepare(), so a
        // missing table could otherwise go unnoticed.
        try
        {
            std::string explainQuery = "EXPLAIN ";
            explainQuery.reserve(metadata.query.size() + 8);
            for (char ch : metadata.query)
            {
                if (ch == '?')
                    explainQuery.append("NULL");
                else
                    explainQuery.push_back(ch);
            }

            auto explain = CreateStatement();
            explain->execute(explainQuery);
        }
        catch (const std::exception& innerEx)
        {
            error = innerEx.what();
            return false;
        }

        return true;
    }
    catch (const sql::SQLException& ex)
    {
        error = ex.what();
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
    }
    catch (...)
    {
        error = "Unknown error";
    }

    return false;
}

bool DatabaseConnection::Ping()
{
    try
    {
        auto statement = CreateStatement();
        statement->execute("SELECT 1");
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_WARNING(std::string("Database ping failed: ") + ex.what());
        return false;
    }
}

std::uint64_t DatabaseConnection::GetLastInsertId()
{
    if (!connection_)
        throw std::runtime_error("Database connection is not established");

    auto lastInsertId = connection_->prepareStatement("SELECT LAST_INSERT_ID()");
    auto result = std::unique_ptr<sql::ResultSet>(lastInsertId->executeQuery());
    if (!result || !result->next())
        throw std::runtime_error("SELECT LAST_INSERT_ID() returned no rows");

    return result->getUInt64(1);
}

void DatabaseConnection::BeginTransaction()
{
    connection_->setAutoCommit(false);
}

void DatabaseConnection::Commit()
{
    connection_->commit();
    connection_->setAutoCommit(true);
}

void DatabaseConnection::Rollback()
{
    connection_->rollback();
    connection_->setAutoCommit(true);
}

std::unique_ptr<sql::Statement> DatabaseConnection::CreateStatement()
{
    if (!connection_)
        throw std::runtime_error("Database connection is not established");
    return std::unique_ptr<sql::Statement>(connection_->createStatement());
}

StatementMetadata DatabaseConnection::LookupMetadata(StatementName name)
{
    return PreparedStatementRegistry::Instance().GetMetadata(name);
}

StatementMetadata DatabaseConnection::LookupMetadata(PreparedStatementIndex name)
{
    return PreparedStatementRegistry::Instance().GetMetadata(static_cast<StatementName>(name));
}

StatementMetadata DatabaseConnection::LookupMetadata(const std::string& alias)
{
    return PreparedStatementRegistry::Instance().GetMetadata(alias);
}

PreparedStatementSharedPtr DatabaseConnection::GetSharedInternal(const StatementMetadata& metadata)
{
    std::lock_guard<std::mutex> lock(preparedMutex_);

    if (!metadata.alias.empty())
    {
        auto aliasIt = sharedByAlias_.find(metadata.alias);
        if (aliasIt != sharedByAlias_.end())
            return aliasIt->second;
    }

    auto nameIt = sharedByName_.find(metadata.name);
    if (nameIt != sharedByName_.end())
        return nameIt->second;

    auto prepared = MakePreparedStatement(connection_.get(), metadata);
    auto shared = PreparedStatementSharedPtr(prepared.release());
    sharedByName_[metadata.name] = shared;
    if (!metadata.alias.empty())
        sharedByAlias_[metadata.alias] = shared;
    return shared;
}

} // namespace database

