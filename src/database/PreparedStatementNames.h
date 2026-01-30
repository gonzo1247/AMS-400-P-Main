#pragma once

#include "DatabaseTypes.h"
#include "Implementation/AMSDatabase.h"
#include "Implementation/IMSDatabase.h"

namespace database
{

enum class PreparedStatementIndex : std::uint32_t
{
    IMS_MAX = static_cast<std::uint32_t>(Implementation::IMSPreparedStatement::MAX_VALUE),


    AMS_ACCOUNT_TEST_PING = static_cast<std::uint32_t>(Implementation::AMSPreparedStatement::ACCOUNT_TEST_PING),
    MAX_VALUE = static_cast<std::uint32_t>(Implementation::AMSPreparedStatement::MAX_VALUE)
};

inline StatementName ToStatementName(PreparedStatementIndex index)
{
    return static_cast<StatementName>(index);
}

inline PreparedStatementIndex ToStatementIndex(StatementName name)
{
    return static_cast<PreparedStatementIndex>(static_cast<std::uint32_t>(name));
}

inline StatementName ToStatementName(Implementation::IMSPreparedStatement statement)
{
    return static_cast<StatementName>(statement);
}

inline StatementName ToStatementName(Implementation::AMSPreparedStatement statement)
{
    return static_cast<StatementName>(statement);
}

} // namespace database

