#pragma once

#include <cstdint>
#include <mariadb/conncpp.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVariant>

#include "Duration.h"

namespace database
{

using FieldValue = std::variant<std::nullptr_t, bool, std::int64_t, std::uint64_t, double, std::string>;

class Field
{
public:
    Field() = default;
    explicit Field(FieldValue value);

    bool IsNull() const;
    template <typename T>
    T Get() const
    {
        if (IsNull())
            return T{};

        return std::get<T>(value_);
    }

    std::uint8_t GetUInt8() const;
    std::uint16_t GetUInt16() const;
    std::uint32_t GetUInt32() const;
    std::uint64_t GetUInt64() const;

    std::int8_t GetInt8() const;
    std::int16_t GetInt16() const;
    std::int32_t GetInt32() const;
    std::int64_t GetInt64() const;

    bool GetBool() const;

    double GetDouble() const;
    float GetFloat() const;

    SystemTimePoint GetDateTime() const;
    std::optional<SystemTimePoint> GetOptionalDateTime() const;

    const std::string& GetString() const;
    const std::string& GetBlob() const;
    const std::string& GetBinary() const;

    std::vector<std::uint8_t> GetBinaryVectorUInt8() const;
    std::vector<std::uint16_t> GetBinaryVectorUInt16() const;
    std::vector<std::uint32_t> GetBinaryVectorUInt32() const;
    std::vector<std::uint64_t> GetBinaryVectorUInt64() const;

    QString GetQString() const;

    std::string ToString() const;
    QString ToQString() const;
    QByteArray ToQByteArray() const;
    QDateTime ToQDateTime(const QString& format = QStringLiteral("yyyy-MM-dd HH:mm:ss")) const;
    QVariant ToQVariant() const;

private:
    FieldValue value_ = nullptr;
};

Field FromResultColumn(sql::ResultSet* result, std::size_t index);

} // namespace database

