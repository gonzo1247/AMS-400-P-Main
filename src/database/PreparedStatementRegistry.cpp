#include "PreparedStatementRegistry.h"

#include <stdexcept>

namespace database
{

PreparedStatementRegistry& PreparedStatementRegistry::Instance()
{
    static PreparedStatementRegistry instance;
    return instance;
}

StatementName PreparedStatementRegistry::RegisterStatement(const std::string& alias, StatementName name, const std::string& query, StatementConnectionType type)
{
    std::lock_guard<std::mutex> lock(mutex_);

    StatementMetadata metadata{ name, alias, query, type };
    byName_[name] = metadata;
    if (!alias.empty())
        byAlias_[alias] = name;

    return name;
}

StatementMetadata PreparedStatementRegistry::GetMetadata(StatementName name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = byName_.find(name);
    if (it == byName_.end())
        throw std::runtime_error("Prepared statement not found");
    return it->second;
}

StatementMetadata PreparedStatementRegistry::GetMetadata(const std::string& alias) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = byAlias_.find(alias);
    if (it == byAlias_.end())
        throw std::runtime_error("Prepared statement alias not found");
    return byName_.at(it->second);
}

std::vector<StatementMetadata> PreparedStatementRegistry::GetAll() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StatementMetadata> result;
    result.reserve(byName_.size());
    for (const auto& [_, metadata] : byName_)
        result.push_back(metadata);
    return result;
}

} // namespace database

