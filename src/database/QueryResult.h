#pragma once

#include <memory>
#include <vector>

#include <mariadb/conncpp.hpp>

#include "Field.h"

namespace database
{

class QueryResult
{
public:
    explicit QueryResult(std::unique_ptr<sql::ResultSet> result);
    QueryResult(std::unique_ptr<sql::ResultSet> result, std::unique_ptr<sql::PreparedStatement> statement);

    bool Next();
    bool NextRow();

    Field* Fetch();
    std::size_t GetFieldCount() const;
    Field GetField(std::size_t index) const;

    bool IsValid() const { return hasRow_; }

private:
    void LoadCurrentRow();

    std::unique_ptr<sql::PreparedStatement> statement_;
    std::unique_ptr<sql::ResultSet> result_;
    std::vector<Field> currentRow_;
    bool hasRow_ = false;
    bool initialConsumed_ = false;
};

} // namespace database

