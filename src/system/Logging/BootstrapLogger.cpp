#include "BootstrapLogger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <format>
#include <string>
#include <shlobj.h> // SHGetKnownFolderPath
#include <combaseapi.h> // CoTaskMemFree

namespace BootstrapLogger
{
	void Log(const std::string& message)
	{
		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm localTime = {};

#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&localTime, &now);
#else
		localtime_r(&now, &localTime);
#endif

		std::stringstream timestamp;
		timestamp << std::put_time(&localTime, "%Y-%m-%d %X");

		std::filesystem::path programDataPath = std::filesystem::path(GetProgramDataPath()) / L"Lager-und-Bestellverwaltung";
		std::filesystem::create_directories(programDataPath);
		std::filesystem::path settingErrorLogPath = programDataPath / "SettingError.log";

		std::ofstream outputFile(settingErrorLogPath.string(), std::ios::app);

		if (outputFile.is_open())
		{
			outputFile << "[" << timestamp.str() << "] " << message << std::endl;
			outputFile.close();
		}
		else
		{
			std::cerr << "[BootstrapLogger] Failed to open SettingError.log for writing." << std::endl;
		}
	}

	std::wstring GetProgramDataPath()
	{
		PWSTR path = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &path)))
		{
			std::wstring result(path);
			CoTaskMemFree(path);
			return result;
		}
		return L"";
	}

}