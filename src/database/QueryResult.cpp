
#include "QueryResult.h"

namespace database
{

QueryResult::QueryResult(std::unique_ptr<sql::ResultSet> result) : QueryResult(std::move(result), nullptr) {}


QueryResult::QueryResult(std::unique_ptr<sql::ResultSet> result, std::unique_ptr<sql::PreparedStatement> statement) : statement_(std::move(statement)), result_(std::move(result))
{
    if (result_ && result_->next())
    {
        LoadCurrentRow();
        hasRow_ = true;
    }
}

bool QueryResult::Next()
{
    if (!hasRow_)
        return false;

    if (!initialConsumed_)
    {
        initialConsumed_ = true;
        return true;
    }

    return NextRow();
}

bool QueryResult::NextRow()
{
    if (!result_)
        return false;

    if (!result_->next())
    {
        result_.reset();
        currentRow_.clear();
        hasRow_ = false;
        return false;
    }

    LoadCurrentRow();
    return true;
}

Field* QueryResult::Fetch()
{
    if (!hasRow_)
        return nullptr;

    return currentRow_.data();
}

std::size_t QueryResult::GetFieldCount() const
{
    return currentRow_.size();
}

Field QueryResult::GetField(std::size_t index) const
{
    if (index >= currentRow_.size())
        return Field{};

    return currentRow_[index];
}

void QueryResult::LoadCurrentRow()
{
    const auto columns = result_->getMetaData()->getColumnCount();
    currentRow_.clear();
    currentRow_.reserve(columns);

    for (std::size_t i = 0; i < columns; ++i)
        currentRow_.push_back(FromResultColumn(result_.get(), i));
}

} // namespace database

