#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "DatabaseTypes.h"

namespace database
{

class PreparedStatementRegistry
{
public:
    static PreparedStatementRegistry& Instance();

    StatementName RegisterStatement(const std::string& alias, StatementName name, const std::string& query, StatementConnectionType type);

    StatementMetadata GetMetadata(StatementName name) const;
    StatementMetadata GetMetadata(const std::string& alias) const;

    std::vector<StatementMetadata> GetAll() const;

private:
    PreparedStatementRegistry() = default;

    struct MetadataKeyHash
    {
        std::size_t operator()(StatementName name) const noexcept
        {
            return static_cast<std::size_t>(name);
        }
    };

    mutable std::mutex mutex_;
    std::unordered_map<StatementName, StatementMetadata, MetadataKeyHash> byName_;
    StatementNameMap byAlias_;
};

namespace detail
{
inline std::string NormalizeAlias(std::string_view value)
{
    auto pos = value.rfind("::");
    if (pos != std::string_view::npos)
        value.remove_prefix(pos + 2);
    return std::string(value);
}
} // namespace detail

#define PREPARE_STATEMENT(enumValue, query, connType) \
    database::PreparedStatementRegistry::Instance().RegisterStatement(database::detail::NormalizeAlias(#enumValue), static_cast<database::StatementName>(enumValue), (query), (connType))

} // namespace database

