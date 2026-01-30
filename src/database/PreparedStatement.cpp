
#include "PreparedStatement.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include "Duration.h"
#include "Logger.h"
#include "LoggerDefines.h"
#include "PreparedStatementRegistry.h"

namespace database
{
    namespace
    {
        std::string FormatDateTime(const SystemTimePoint& timePoint)
        {
            auto time = std::chrono::system_clock::to_time_t(timePoint);
            std::tm timeInfo{};
            #ifdef _WIN32
            localtime_s(&timeInfo, &time);
            #else
                localtime_r(&time, &timeInfo);
            #endif

            std::ostringstream stream;
            stream << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
            return stream.str();
        }
    }  // namespace

PreparedStatement::PreparedStatement(sql::Connection* connection, const StatementMetadata& metadata, std::unique_ptr<sql::PreparedStatement> statement)
    : connection_(connection), metadata_(metadata), statement_(std::move(statement))
{
}

PreparedStatement::~PreparedStatement() = default;

sql::PreparedStatement* PreparedStatement::GetRaw()
{
    return statement_.get();
}

void PreparedStatement::SetBool(std::size_t index, bool value)
{
    statement_->setBoolean(index + 1, value);
}

void PreparedStatement::SetInt(std::size_t index, std::int32_t value)
{
    statement_->setInt(index + 1, value);
}

void PreparedStatement::SetUInt(std::size_t index, std::uint32_t value)
{
    statement_->setUInt(index + 1, value);
}

void PreparedStatement::SetInt64(std::size_t index, std::int64_t value)
{
    statement_->setInt64(index + 1, value);
}

void PreparedStatement::SetUInt64(std::size_t index, std::uint64_t value)
{
    statement_->setUInt64(index + 1, value);
}

void PreparedStatement::SetDouble(std::size_t index, double value)
{
    statement_->setDouble(index + 1, value);
}

void PreparedStatement::SetBinary(std::size_t index, const std::string& value)
{
    if (binaryStreams_.size() <= index)
        binaryStreams_.resize(index + 1);

    auto stream = std::make_unique<std::istringstream>(value, std::ios::binary);
    statement_->setBlob(index + 1, stream.get());
    binaryStreams_[index] = std::move(stream);
}

void PreparedStatement::SetBinary(std::size_t index, const void* data, std::size_t byteCount)
{
    if (data == nullptr || byteCount == 0)
    {
        SetBinary(index, std::string{});
        return;
    }

    const auto* bytes = static_cast<const char*>(data);
    SetBinary(index, std::string(bytes, bytes + byteCount));
}

void PreparedStatement::SetString(std::size_t index, const std::string& value)
{
    statement_->setString(index + 1, sql::SQLString(value));
}

void PreparedStatement::SetQString(std::size_t index, const QString& value)
{
    SetString(index, value.toStdString());
}

void PreparedStatement::SetQByteArray(std::size_t index, const QByteArray& value)
{
    SetBinary(index, std::string(value.constData(), static_cast<std::size_t>(value.size())));
}

void PreparedStatement::SetQDateTime(std::size_t index, const QDateTime& value)
{
    statement_->setDateTime(index + 1, sql::SQLString(value.toString("yyyy-MM-dd HH:mm:ss").toStdString()));
}

void PreparedStatement::SetQVariant(std::size_t index, const QVariant& value)
{
    if (!value.isValid() || value.isNull())
    {
        SetNull(index);
        return;
    }

    switch (value.typeId())
    {
        case QMetaType::Bool:
            SetBool(index, value.toBool());
            return;
        case QMetaType::Int:
            SetInt(index, value.toInt());
            return;
        case QMetaType::UInt:
            SetUInt(index, value.toUInt());
            return;
        case QMetaType::LongLong:
            SetInt64(index, value.toLongLong());
            return;
        case QMetaType::ULongLong:
            SetUInt64(index, value.toULongLong());
            return;
        case QMetaType::Double:
            SetDouble(index, value.toDouble());
            return;
        case QMetaType::QString:
            SetQString(index, value.toString());
            return;
        case QMetaType::QByteArray:
            SetQByteArray(index, value.toByteArray());
            return;
        case QMetaType::QDateTime:
            SetQDateTime(index, value.toDateTime());
            return;
        default:
            LOG_DEBUG("SetQVariant: unhandled type {}, binding as string", value.typeName());
            SetQString(index, value.toString());
            return;
    }
}

void PreparedStatement::SetNull(std::size_t index)
{
    statement_->setNull(index + 1, sql::DataType::SQLNULL);
}

void PreparedStatement::SetSystemPointTime(std::size_t index, const SystemTimePoint& value)
{
    statement_->setDateTime(index + 1, sql::SQLString(FormatDateTime(value)));
}

void PreparedStatement::SetCurrentDate(std::size_t index)
{
    auto now = std::chrono::system_clock::now();
    statement_->setDateTime(index + 1, sql::SQLString(FormatDateTime(now)));
}

PreparedStatementPtr MakePreparedStatement(sql::Connection* connection, const StatementMetadata& metadata)
{
    try
    {
        auto* rawStmt = connection->prepareStatement(metadata.query);
        return std::make_unique<PreparedStatement>(connection, metadata,
                                                   std::unique_ptr<sql::PreparedStatement>(rawStmt));
    }
    catch (sql::SQLException& ex)
    {
        LOG_SQL("Failed to prepare SQL statement\n"
            "Query: {}\n"
            "Name: {}\n"
            "Error: {}\n"
            "Code: {}\n"
            "State: {}",
            metadata.query, metadata.name, ex.what(), ex.getErrorCode(), ex.getSQLState());

        throw;  // preserve original exception + stack
    }
}

} // namespace database

