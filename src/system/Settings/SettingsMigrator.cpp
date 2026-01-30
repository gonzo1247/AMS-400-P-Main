#include "SettingsMigrator.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#include "SharedDefines.h"

bool SettingsMigrator::TryMigrateLegacySettings()
{
	if (!std::filesystem::exists(legacySettingsPath))
		return false;

	if (std::filesystem::exists(migratedSettingsPath))
		return false;

	auto legacyData = LoadLegacySettings();
	MigrateToQSettings(legacyData);
//	BackupLegacyFile();
	return true;
}

std::map<std::string, std::string> SettingsMigrator::LoadLegacySettings()
{
	std::map<std::string, std::string> result;
	std::ifstream file(legacySettingsPath);
	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value))
		{
			// Trim enclosing quotes if present
			if (!value.empty() && value.front() == '"' && value.back() == '"')
				value = value.substr(1, value.length() - 2);

			result[key] = value;
		}
	}
	return result;
}

void SettingsMigrator::MigrateToQSettings(const std::map<std::string, std::string>& legacy)
{
	QSettings settings(migratedSettingsPath, QSettings::IniFormat);

	auto getSafe = [&](const std::string& key, const QString& fallback = "") -> QString {
		auto it = legacy.find(key);
		return it != legacy.end() ? QString::fromStdString(it->second) : fallback;
		};

	auto getBool = [&](const std::string& key, bool fallback = false) -> bool {
		auto it = legacy.find(key);
		if (it == legacy.end()) return fallback;
		const std::string& val = it->second;
		return val == "1" || val == "true" || val == "True" || val == "TRUE";
		};

	// Logging
	settings.beginGroup("Logging");
	settings.setValue("LogPath", getSafe("LogFilePath"));
	settings.setValue("Prefix", getSafe("LogNamePrefixIMS"));
	QString logLevels = getSafe("EnabledLogLevel");
	logLevels.replace('"', "");
	settings.setValue("EnabledLogLevel", logLevels);
	settings.endGroup();

	// MySQL
	settings.beginGroup("MySQL");
	settings.setValue("Hostname", getSafe("MYSQLHostname"));
	settings.setValue("Username", getSafe("MYSQLUsername"));
	settings.setValue("Password", getSafe("MYSQLPassword"));
	settings.setValue("Port", getSafe("MYSQLPort"));
	settings.setValue("Database", getSafe("MYSQLDatabaseName"));
	settings.setValue("IsActive", getBool("MYSQLIsActive"));
	settings.endGroup();

	// Label
	if (legacy.contains("LabelWidthMM"))
	{
		settings.beginGroup("LabelLayout");
		settings.setValue("WidthMM", getSafe("LabelWidthMM"));
		settings.setValue("HeightMM", getSafe("LabelHeightMM"));
		settings.setValue("FontSizePt", getSafe("LabelFontSizePt"));
		settings.endGroup();
	}

	// Sound
	settings.beginGroup("Sound");
	settings.setValue("Volume", getSafe("SoundVolume"));
	settings.setValue("Enabled", getBool("SoundEnabled"));
	settings.endGroup();

	// Language
	settings.beginGroup("Language");
	settings.setValue("Selected", getSafe("Language"));
	settings.endGroup();

	// Display
	settings.beginGroup("Display");
	settings.setValue("FullScreen", getBool("FullScreen"));
	settings.setValue("PersonalStyle", getBool("PersonalStyle"));
	settings.setValue("PersonalLanguage", getBool("PersonalLanguage"));
	settings.setValue("ShowVirtualKeyboard", getBool("ShowVirtualKeyboard"));
	settings.endGroup();

	// Communication
	settings.beginGroup("Communication");
	settings.setValue("LocalPort", getSafe("PortLocalCommunication"));
	settings.endGroup();

	// Chip Reader
	settings.beginGroup("ChipReader");
	settings.setValue("Port", getSafe("ChipSerialPortName"));
	settings.setValue("BaudRate", getSafe("ChipBaudRate"));
	settings.setValue("DataBits", getSafe("ChipDataBits"));
	settings.setValue("Parity", getSafe("ChipParity"));
	settings.setValue("StopBits", getSafe("ChipStopBits"));
	settings.setValue("FlowControl", getSafe("ChipFlowControl"));
	settings.endGroup();

	// Barcode Reader
	settings.beginGroup("BarcodeReader");
	settings.setValue("Port", getSafe("BarcodeSerialPortName"));
	settings.setValue("BaudRate", getSafe("BarcodeBaudRate"));
	settings.setValue("DataBits", getSafe("BarcodeDataBits"));
	settings.setValue("Parity", getSafe("BarcodeParity"));
	settings.setValue("StopBits", getSafe("BarcodeStopBits"));
	settings.setValue("FlowControl", getSafe("BarcodeFlowControl"));
	settings.endGroup();

	// Company
	settings.beginGroup("Company");
	settings.setValue("Location", getSafe("CompanyLocation"));
	settings.endGroup();

	// Style
	settings.beginGroup("Style");
	settings.setValue("Selected", getSafe("StyleThema"));
	settings.endGroup();

	// Order
	settings.beginGroup("Order");
	settings.setValue("ConfirmationFilePath", getSafe("OrderConfirmationFilePath"));
	settings.endGroup();

	// SSH
	settings.beginGroup("SSH");
	settings.setValue("Hostname", getSafe("SSH_Hostname"));
	settings.setValue("Port", getSafe("SSH_Port"));
	settings.setValue("Username", getSafe("SSH_Username"));
	settings.setValue("ProgrammPath", getSafe("SSH_ProgrammPath"));
	settings.setValue("IsActive", getBool("SSH_IsActive"));
	settings.setValue("LocalPort", getSafe("SSHLocalPort"));
	settings.endGroup();

	// Window geometry
	for (const auto& [key, value] : legacy)
	{
		WindowNames winEnum = GetWindowsEnum(key);
		if (winEnum == WindowNames::WINDOW_NONE)
			continue;

		const std::string& val = value;
		auto sizeDelimiter = val.find("::");
		auto posDelimiter = val.rfind(";");

		if (sizeDelimiter == std::string::npos || posDelimiter == std::string::npos)
			continue;

		try
		{
			int height = std::stoi(val.substr(0, val.find(";")));
			int width = std::stoi(val.substr(val.find(";") + 1, sizeDelimiter - val.find(";") - 1));
			int x = std::stoi(val.substr(sizeDelimiter + 2, posDelimiter - sizeDelimiter - 2));
			int y = std::stoi(val.substr(posDelimiter + 1));

			settings.beginGroup("Window");
			settings.beginGroup(QString::fromStdString(key));

			settings.setValue("Width", width);
			settings.setValue("Height", height);
			settings.setValue("X", x);
			settings.setValue("Y", y);

			settings.endGroup(); // window name
			settings.endGroup(); // "Window"
		}
		catch (...) {}
	}
}

void SettingsMigrator::BackupLegacyFile()
{
	std::filesystem::path oldPath(legacySettingsPath);
	std::filesystem::path newPath = oldPath;
	newPath += ".bak";

	std::error_code ec;
	std::filesystem::rename(oldPath, newPath, ec);
}
