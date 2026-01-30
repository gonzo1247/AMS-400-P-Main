#pragma once
#include <QSettings>
#include <string>
#include <map>

class SettingsMigrator
{
public:
	static bool TryMigrateLegacySettings();

private:
	static constexpr const char* legacySettingsPath = "C:/ProgramData/Lager-und-Bestellverwaltung/settings.sett";
	static constexpr const char* migratedSettingsPath = "C:/ProgramData/Lager-und-Bestellverwaltung/settings.ini";

	static std::map<std::string, std::string> LoadLegacySettings();
	static void MigrateToQSettings(const std::map<std::string, std::string>& legacy);
	static void BackupLegacyFile();
};
