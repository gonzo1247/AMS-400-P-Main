#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <sstream>

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVariant>

#include <mariadb/conncpp.hpp>

#include "DatabaseTypes.h"
#include "Duration.h"

namespace database
{

class PreparedStatement
{
public:
    PreparedStatement(sql::Connection* connection, const StatementMetadata& metadata, std::unique_ptr<sql::PreparedStatement> statement);
    ~PreparedStatement();

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    PreparedStatement(PreparedStatement&&) noexcept = default;
    PreparedStatement& operator=(PreparedStatement&&) noexcept = default;

    sql::PreparedStatement* GetRaw();

    const StatementMetadata& GetMetadata() const { return metadata_; }

    void SetBool(std::size_t index, bool value);
    void SetInt(std::size_t index, std::int32_t value);
    void SetUInt(std::size_t index, std::uint32_t value);
    void SetInt64(std::size_t index, std::int64_t value);
    void SetUInt64(std::size_t index, std::uint64_t value);
    void SetDouble(std::size_t index, double value);
    void SetBinary(std::size_t index, const std::string& value);
    void SetBinary(std::size_t index, const void* data, std::size_t byteCount);

    template <typename Integral>
    void SetBinary(std::size_t index, const std::vector<Integral>& value)
    {
        static_assert(std::is_integral_v<Integral>, "SetBinary expects integral vector types");
        SetBinary(index, value.data(), value.size() * sizeof(Integral));
    }
    void SetString(std::size_t index, const std::string& value);
    void SetQString(std::size_t index, const QString& value);
    void SetQByteArray(std::size_t index, const QByteArray& value);
    void SetQDateTime(std::size_t index, const QDateTime& value);
    void SetQVariant(std::size_t index, const QVariant& value);
    void SetCurrentDate(std::size_t index);
    void SetNull(std::size_t index);
    void SetSystemPointTime(std::size_t index, const SystemTimePoint& value);

private:
    sql::Connection* connection_;
    StatementMetadata metadata_;
    std::unique_ptr<sql::PreparedStatement> statement_;
    std::vector<std::unique_ptr<std::istringstream>> binaryStreams_;
};

PreparedStatementPtr MakePreparedStatement(sql::Connection* connection, const StatementMetadata& metadata);

} // namespace database

