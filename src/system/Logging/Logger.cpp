/*
Copyright 2023 - 2024 by Severin

*/

#include <format>

#include "Logger.h"

#include <ranges>

#include "SettingsManager.h"

Logger::Logger(LoggerTypes types) : _loggerTypes(types)
{
//	_errorHandler = std::make_unique<UIErrorHandler>();

	OpenOrCreateLogFile();
}

Logger::Logger() : _loggerTypes(LoggerTypes::LOG_TYPE_MISC)
{
//	_errorHandler = std::make_unique<UIErrorHandler>();
	OpenOrCreateLogFile();
}

Logger::~Logger()
{
	CloseLogFile();
}

Logger* Logger::instance()
{
	static Logger instance;
	return &instance;
}

void Logger::Init()
{
	(void)instance();
}

void Logger::OpenOrCreateLogFile()
{
	const std::string basePath = ensurePath(GetSettings().getLogPath());

	// Acquire the lock once for the entire operation
	std::lock_guard<std::mutex> lock(_mutex);

	for (auto loggerType : { LoggerTypes::LOG_TYPE_SQL, LoggerTypes::LOG_TYPE_MISC, LoggerTypes::LOG_TYPE_ERROR, LoggerTypes::LOG_TYPE_DEBUG })
	{
		std::tm localTime = GetCurrentLocalTime();

		std::ostringstream filename;
		filename << basePath << "/log_";

		if (loggerType == LoggerTypes::LOG_TYPE_SQL)
			filename << "sql_";
		else if (loggerType == LoggerTypes::LOG_TYPE_MISC)
			filename << "misc_";
		else if (loggerType == LoggerTypes::LOG_TYPE_ERROR)
			filename << "error_";
		else if (loggerType == LoggerTypes::LOG_TYPE_DEBUG)
			filename << "debug_";

		filename << std::put_time(&localTime, "%Y-%m-%d") << ".txt";
		
		auto& logFile = _logFiles[loggerType];
		logFile.open(filename.str(), std::ios::app);

		if (!logFile.is_open())
		{
	//		_errorHandler->SendWarningDialog(ErrorCodes::ERROR_LOG_FILE_NOT_OPEN_OR_CREATED);
		}
	}
}

void Logger::WriteLogFile(LoggerTypes const type, std::string_view format_str)
{
/*
	if (!IsLogLevelEnabled(level))
		return;*/

	std::lock_guard<std::mutex> lock(_mutex);

	const auto it = _logFiles.find(type);
	if (it == _logFiles.end() || !it->second.is_open())
	{
//		_errorHandler->SendWarningDialog(ErrorCodes::ERROR_LOG_FILE_NOT_OPEN_OR_CREATED);
		return;
	}

	const std::tm localTime = GetCurrentLocalTime();

	std::ostringstream logMessage;
	logMessage << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ": "
		<< format_str << "\n";

	it->second << logMessage.str();
	it->second.flush();
}

void Logger::WriteLogFile(LoggerTypes type, LogLevel level, const std::string& formattedMessage)
{
	if (!IsLogLevelEnabled(level))
		return;

	std::lock_guard<std::mutex> lock(_mutex);

	const auto it = _logFiles.find(type);
	if (it == _logFiles.end() || !it->second.is_open())
	{
//		_errorHandler->SendWarningDialog(ErrorCodes::ERROR_LOG_FILE_NOT_OPEN_OR_CREATED);
		return;
	}

	const std::tm localTime = GetCurrentLocalTime();

	try
	{
		std::ostringstream logMessage;
		logMessage << GetLogLevelString(level);
		logMessage << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ": "
			<< formattedMessage << "\n";

		it->second << logMessage.str();
		it->second.flush();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in WriteLogFile: " << e.what() << std::endl;
	}
}

bool Logger::IsLogLevelEnabled(LogLevel level)
{
	return GetSettings().isLogLevelEnabled(level);
}

void Logger::CloseLogFile()
{
	if (_logFile.is_open())
		_logFile.close();

}

void Logger::CloseAllLogFiles()
{
	std::lock_guard<std::mutex> lock(_mutex);
	for (auto& file : _logFiles | std::views::values)
	{
		if (file.is_open())
			file.close();
	}
}

std::string Logger::ensurePath(const std::string& desiredPath) const
{
	static std::unordered_map<std::string, bool> validatedPaths;
	const auto it = validatedPaths.find(desiredPath);

	if (it != validatedPaths.end() && it->second)
		return desiredPath;

	const fs::path primaryPath = "C:\\ProgramData\\Lager-und-Bestellverwaltung\\logs";

	char* userProfile = nullptr;
	size_t len = 0;
	const fs::path fallbackPath = fs::path(userProfile ? userProfile : "") / "Documents" / "Lager-und-Bestellverwaltung" / "logs";
	if (userProfile) free(userProfile);

	auto createDirectoryIfNotExists = [](const fs::path& path) -> bool {
		try
		{
			if (!fs::exists(path))
			{
				fs::create_directories(path);
			}
			return true;
		}
		catch (const fs::filesystem_error& e)
		{
			return false;
		}
		};

	if (createDirectoryIfNotExists(desiredPath))
	{
		validatedPaths[desiredPath] = true;
		return desiredPath;
	}

	if (createDirectoryIfNotExists(primaryPath))
	{
		validatedPaths[primaryPath.string()] = true;
		return primaryPath.string();
	}

	if (createDirectoryIfNotExists(fallbackPath))
	{
		validatedPaths[fallbackPath.string()] = true;
		return fallbackPath.string();
	}

	return "";
}

void Logger::OutMessageImpl(LoggerTypes level, std::string message)
{
	// WriteLogFile(level, format_str, std::make_format_args(args...));
}

std::tm Logger::GetCurrentLocalTime()
{
	std::time_t now = std::time(nullptr);
	std::tm localTime = {};

#ifdef _WIN32
	if (localtime_s(&localTime, &now) != 0)
	{
		return {};
	}
#else
	if (localtime_r(&now, &localTime) == nullptr)
	{
		return {};
	}
#endif

	return localTime;
}

std::string Logger::GetLogLevelString(const LogLevel level)
{
	switch (level)
	{
		case LOG_LEVEL_TRACE: return "<TRACE> ";
		case LOG_LEVEL_DEBUG: return "<DEBUG> ";
		case LOG_LEVEL_INFO: return "<INFO> ";
		case LOG_LEVEL_WARNING: return "<WARNING> ";
		case LOG_LEVEL_ERROR: return "<ERROR> ";
		case LOG_LEVEL_FATAL: return "<FATAL> ";
	}
	return {};
}
