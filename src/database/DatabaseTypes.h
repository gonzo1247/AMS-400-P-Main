#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace database
{

enum class ConnectionType
{
    Sync,
    Async
};

enum class StatementConnectionType
{
    Sync,
    Async,
    Both
};

constexpr StatementConnectionType CONNECTION_SYNC = StatementConnectionType::Sync;
constexpr StatementConnectionType CONNECTION_ASYNC = StatementConnectionType::Async;
constexpr StatementConnectionType CONNECTION_BOTH = StatementConnectionType::Both;

enum class StatementName : std::uint32_t
{
    NONE = 0,
};

struct StatementMetadata
{
    StatementName name = StatementName::NONE;
    std::string alias;
    std::string query;
    StatementConnectionType connectionType = StatementConnectionType::Sync;
};

using StatementNameMap = std::unordered_map<std::string, StatementName>;

class PreparedStatement;
class PreparedStatementHandle;
class DatabaseConnection;
class ConnectionPool;
class QueryResult;

using PreparedStatementPtr = std::unique_ptr<PreparedStatement>;
using PreparedStatementSharedPtr = std::shared_ptr<PreparedStatement>;

} // namespace database

