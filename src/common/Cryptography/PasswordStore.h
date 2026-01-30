#pragma once

#include <QString>

class PasswordStore
{
public:
	static bool MigratePasswordFromBin();
	static std::optional<std::string> LoadPassword();
	static bool SavePassword(const std::string& plainPassword);

private:
	static constexpr const wchar_t* credTargetName = L"IMS_MySQL_Password";

	static constexpr wchar_t CRED_UUID_PASSWORD[] = L"{C60F3967-6A3F-44E1-B1B6-5F33E8BE9C55}";
	static constexpr wchar_t CRED_UUID_AESKEY[] = L"{C9D86125-EC6E-4EF4-B9A5-463DA3EAE8B7}";
};

