#include "Field.h"

#include <limits>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>

namespace database
{

namespace
{

const std::string kEmptyString{};

std::string SqlStringToStdString(const sql::SQLString& sqlString)
{
    return std::string(sqlString.begin(), sqlString.end());
}

std::string ReadStream(std::unique_ptr<std::istream> stream)
{
    if (!stream)
        return {};

    return std::string(std::istreambuf_iterator<char>(*stream), std::istreambuf_iterator<char>());
}

std::optional<SystemTimePoint> ParseDateTimeString(const std::string& value)
{
    std::tm timeInfo{};
    std::istringstream stream(value);
    stream >> std::get_time(&timeInfo, "%Y-%m-%d %H:%M:%S");

    if (stream.fail())
        return std::nullopt;

    auto timestamp = std::mktime(&timeInfo);
    if (timestamp == -1)
        return std::nullopt;

    return std::chrono::system_clock::from_time_t(timestamp);
}

template <typename T>
std::vector<T> GetBinaryVector(const std::string& data)
{
    if (data.empty())
        return {};

    if (data.size() % sizeof(T) != 0)
        throw std::runtime_error("Field binary size does not align with target type");

    const std::size_t elementCount = data.size() / sizeof(T);
    std::vector<T> result(elementCount);
    std::memcpy(result.data(), data.data(), data.size());
    return result;
}

template <typename Target, typename Source>
Target CheckedIntegralCast(Source value)
{
    if constexpr (std::is_same_v<Target, Source>)
    {
        return value;
    }

    if constexpr (std::is_signed_v<Source>)
    {
        if constexpr (std::is_signed_v<Target>)
        {
            if (value < static_cast<Source>(std::numeric_limits<Target>::min()) ||
                value > static_cast<Source>(std::numeric_limits<Target>::max()))
                throw std::out_of_range("Field integral cast out of range");
        }
        else
        {
            if (value < 0)
                throw std::out_of_range("Field integral cast out of range");

            using UnsignedSource = std::make_unsigned_t<Source>;
            if (static_cast<UnsignedSource>(value) > std::numeric_limits<Target>::max())
                throw std::out_of_range("Field integral cast out of range");
        }
    }
    else
    {
        if constexpr (std::is_signed_v<Target>)
        {
            if (value > static_cast<Source>(std::numeric_limits<Target>::max()))
                throw std::out_of_range("Field integral cast out of range");
        }
        else if (value > std::numeric_limits<Target>::max())
        {
            throw std::out_of_range("Field integral cast out of range");
        }
    }

    return static_cast<Target>(value);
}

template <typename Target>
Target GetIntegralValue(const FieldValue& value)
{
    return std::visit(
        [](const auto& stored) -> Target {
            using StoredType = std::decay_t<decltype(stored)>;

            if constexpr (std::is_same_v<StoredType, std::nullptr_t>)
            {
                throw std::bad_variant_access{};
            }
            else if constexpr (std::is_same_v<StoredType, bool>)
            {
                return CheckedIntegralCast<Target>(static_cast<std::uint32_t>(stored));
            }
            else if constexpr (std::is_same_v<StoredType, std::int64_t> ||
                               std::is_same_v<StoredType, std::uint64_t>)
            {
                return CheckedIntegralCast<Target>(stored);
            }
            else
            {
                throw std::bad_variant_access{};
            }
        },
        value);
}

}

Field::Field(FieldValue value) : value_(std::move(value)) {}

bool Field::IsNull() const
{
    return std::holds_alternative<std::nullptr_t>(value_);
}

std::string Field::ToString() const
{
    if (IsNull())
        return "NULL";

    if (std::holds_alternative<std::string>(value_))
        return std::get<std::string>(value_);

    if (std::holds_alternative<bool>(value_))
        return std::get<bool>(value_) ? "1" : "0";

    if (std::holds_alternative<std::int64_t>(value_))
        return std::to_string(std::get<std::int64_t>(value_));

    if (std::holds_alternative<std::uint64_t>(value_))
        return std::to_string(std::get<std::uint64_t>(value_));

    if (std::holds_alternative<double>(value_))
        return std::to_string(std::get<double>(value_));

    return {};
}

QString Field::ToQString() const
{
    return QString::fromStdString(ToString());
}

QByteArray Field::ToQByteArray() const
{
    return QByteArray::fromStdString(ToString());
}

QDateTime Field::ToQDateTime(const QString& format) const
{
    return QDateTime::fromString(ToQString(), format);
}

QVariant Field::ToQVariant() const
{
    return std::visit(
        []<typename T0>(const T0& stored) -> QVariant
        {
            using StoredType = std::decay_t<T0>;

            if constexpr (std::is_same_v<StoredType, std::nullptr_t>)
            {
                return QVariant{};
            }
            else if constexpr (std::is_same_v<StoredType, bool>)
            {
                return QVariant::fromValue(stored);
            }
            else if constexpr (std::is_same_v<StoredType, std::int64_t>)
            {
                return QVariant::fromValue(static_cast<qlonglong>(stored));
            }
            else if constexpr (std::is_same_v<StoredType, std::uint64_t>)
            {
                return QVariant::fromValue(static_cast<qulonglong>(stored));
            }
            else if constexpr (std::is_same_v<StoredType, double>)
            {
                return QVariant::fromValue(stored);
            }
            else if constexpr (std::is_same_v<StoredType, std::string>)
            {
                return QVariant(QString::fromStdString(stored));
            }
        },
        value_);
}

std::uint8_t Field::GetUInt8() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::uint8_t>(value_);
}

std::uint16_t Field::GetUInt16() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::uint16_t>(value_);
}

std::uint32_t Field::GetUInt32() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::uint32_t>(value_);
}

std::uint64_t Field::GetUInt64() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::uint64_t>(value_);
}

std::int8_t Field::GetInt8() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::int8_t>(value_);
}

std::int16_t Field::GetInt16() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::int16_t>(value_);
}

std::int32_t Field::GetInt32() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::int32_t>(value_);
}

std::int64_t Field::GetInt64() const
{
    if (IsNull())
        return 0;

    return GetIntegralValue<std::int64_t>(value_);
}

bool Field::GetBool() const
{
    if (IsNull())
        return false;

    return std::visit([]<typename T0>(const T0& v) -> bool
        {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, bool>)
            {
                return v;
            }
            else if constexpr (std::is_integral_v<T>)
            {
                return v != 0;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                std::string s = v;
                s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; }),
                        s.end());

                std::transform(s.begin(), s.end(), s.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (s == "1" || s == "true" || s == "yes" || s == "y")
                    return true;

                return false;
            }
            else
            {
                return false;
            }
        },
        value_);
}

double Field::GetDouble() const
{
    if (IsNull())
        return 0.0;

    return std::visit(
        [](const auto& stored) -> double {
            using StoredType = std::decay_t<decltype(stored)>;

            if constexpr (std::is_same_v<StoredType, std::nullptr_t> ||
                          std::is_same_v<StoredType, std::string>)
            {
                throw std::bad_variant_access{};
            }
            else if constexpr (std::is_same_v<StoredType, double>)
            {
                return stored;
            }
            else if constexpr (std::is_same_v<StoredType, bool>)
            {
                return stored ? 1.0 : 0.0;
            }
            else
            {
                return static_cast<double>(stored);
            }
        },
        value_);
}

float Field::GetFloat() const
{
    return static_cast<float>(GetDouble());
}

const std::string& Field::GetString() const
{
    if (IsNull())
        return kEmptyString;

    return std::get<std::string>(value_);
}

const std::string& Field::GetBlob() const
{
    return GetString();
}

const std::string& Field::GetBinary() const
{
    return GetString();
}

SystemTimePoint Field::GetDateTime() const
{
    return std::visit(
        []<typename T0>(const T0& stored) -> SystemTimePoint
        {
            using StoredType = std::decay_t<T0>;

            if constexpr (std::is_same_v<StoredType, std::string>)
            {
                auto parsed = ParseDateTimeString(stored);
                if (!parsed)
                    throw std::runtime_error("Field does not contain a parseable datetime value");

                return *parsed;
            }
            else if constexpr (std::is_same_v<StoredType, std::nullptr_t>)
            {
                return SystemTimePoint{};
            }
            else
            {
                throw std::bad_variant_access{};
            }
        },
        value_);
}

std::vector<std::uint8_t> Field::GetBinaryVectorUInt8() const
{
    if (IsNull())
        return {};

    return GetBinaryVector<std::uint8_t>(GetBinary());
}

std::vector<std::uint16_t> Field::GetBinaryVectorUInt16() const
{
    if (IsNull())
        return {};

    return GetBinaryVector<std::uint16_t>(GetBinary());
}

std::vector<std::uint32_t> Field::GetBinaryVectorUInt32() const
{
    if (IsNull())
        return {};

    return GetBinaryVector<std::uint32_t>(GetBinary());
}

std::vector<std::uint64_t> Field::GetBinaryVectorUInt64() const
{
    if (IsNull())
        return {};

    return GetBinaryVector<std::uint64_t>(GetBinary());
}

std::optional<SystemTimePoint> Field::GetOptionalDateTime() const
{
    return std::visit(
        []<typename T0>(const T0& stored) -> std::optional<SystemTimePoint>
        {
            using StoredType = std::decay_t<T0>;

            if constexpr (std::is_same_v<StoredType, std::string>)
            {
                auto parsed = ParseDateTimeString(stored);
                if (!parsed)
                    throw std::runtime_error("Field does not contain a parseable datetime value");

                return *parsed;
            }
            else if constexpr (std::is_same_v<StoredType, std::nullptr_t>)
            {
                return std::nullopt;
            }
            else
            {
                throw std::bad_variant_access{};
            }
        },
        value_);
}

QString Field::GetQString() const
{
    if (IsNull())
        return QString::fromStdString(kEmptyString);

    return QString::fromStdString(std::get<std::string>(value_));
}

Field FromResultColumn(sql::ResultSet* result, std::size_t index)
{
    if (!result)
        return Field{};

    const auto meta = result->getMetaData();
    const auto type = meta->getColumnType(index + 1);

    if (result->isNull(index + 1))
        return Field{};

    const bool isSigned = meta->isSigned(index + 1);

    switch (type)
    {
    case sql::DataType::BIT:
    case sql::DataType::BOOLEAN:
        return Field(result->getBoolean(index + 1));
    case sql::DataType::INTEGER:
    case sql::DataType::TINYINT:
    case sql::DataType::SMALLINT:
        if (isSigned)
            return Field(static_cast<std::int64_t>(result->getInt(index + 1)));
        return Field(static_cast<std::uint64_t>(result->getUInt(index + 1)));
    case sql::DataType::BIGINT:
        if (isSigned)
            return Field(static_cast<std::int64_t>(result->getInt64(index + 1)));
        return Field(static_cast<std::uint64_t>(result->getUInt64(index + 1)));
    case sql::DataType::DOUBLE:
    case sql::DataType::FLOAT:
    case sql::DataType::DECIMAL:
    case sql::DataType::NUMERIC:
    case sql::DataType::REAL:
        return Field(static_cast<double>(result->getDouble(index + 1)));
    case sql::DataType::BLOB:
    case sql::DataType::BINARY:
    case sql::DataType::VARBINARY:
    case sql::DataType::LONGVARBINARY:
    {
        return Field(ReadStream(std::unique_ptr<std::istream>(result->getBlob(index + 1))));
    }
    case sql::DataType::VARCHAR:
    case sql::DataType::CHAR:
    case sql::DataType::LONGVARCHAR:
    case sql::DataType::LONGNVARCHAR:
    case sql::DataType::NVARCHAR:
    case sql::DataType::NCHAR:
    case sql::DataType::CLOB:
    case sql::DataType::NCLOB:
    default:
        return Field(SqlStringToStdString(result->getString(index + 1)));
    }
}

} // namespace database

