// LoggerDefines.h
#pragma once

#include <cstdint> // for std::uint32_t and std::uint8_t

// Enum for different logger categories
enum class LoggerTypes : std::uint32_t
{
	LOG_TYPE_SQL                = 1,
	LOG_TYPE_MISC               = 2,
	LOG_TYPE_DEBUG              = 3,
	LOG_TYPE_ERROR              = 4,
};

// Enum for log message severity levels
enum LogLevel : std::uint8_t
{
	LOG_LEVEL_TRACE             = 1,
	LOG_LEVEL_DEBUG             = 2,
	LOG_LEVEL_INFO              = 3,
	LOG_LEVEL_WARNING           = 4,
	LOG_LEVEL_ERROR             = 5,
	LOG_LEVEL_FATAL             = 6,
};

// Singleton access macro
#define sLog Logger::instance()

// Internal macro to format and log a message
#define TC_LOG_MESSAGE_BODY(type__, level__, format__, ...) \
    do { \
        sLog->OutMessage(type__, level__, format__, ##__VA_ARGS__); \
    } while (0)

// Macro for logging miscellaneous information
#define LOG_MISC(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_INFO, format__, ##__VA_ARGS__)

// Macro for logging debug information
#define LOG_DEBUG(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_DEBUG, LogLevel::LOG_LEVEL_DEBUG, format__, ##__VA_ARGS__)

// Macro for logging SQL queries and related operations
#define LOG_SQL(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_SQL, LogLevel::LOG_LEVEL_INFO, format__, ##__VA_ARGS__)

// Macro for detailed tracing logs
#define LOG_TRACE(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_TRACE, format__, ##__VA_ARGS__)

// Macro for general informational messages
#define LOG_INFO(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_INFO, format__, ##__VA_ARGS__)

// Macro for warning messages
#define LOG_WARNING(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_WARNING, format__, ##__VA_ARGS__)

// Macro for error messages
#define LOG_ERROR(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_ERROR, format__, ##__VA_ARGS__)

// Macro for fatal errors
#define LOG_FATAL(format__, ...) \
    TC_LOG_MESSAGE_BODY(LoggerTypes::LOG_TYPE_MISC, LogLevel::LOG_LEVEL_FATAL, format__, ##__VA_ARGS__)
