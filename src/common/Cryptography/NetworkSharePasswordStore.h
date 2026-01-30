#pragma once

#include <optional>
#include <string>

class NetworkSharePasswordStore
{
   public:
    static std::optional<std::string> LoadPassword();
    static bool SavePassword(const std::string& plainPassword);
    static bool DeletePassword();

   private:
    // Separate GUIDs from MySQL password store
    static constexpr wchar_t CRED_UUID_PASSWORD[] = L"{A5C7C8F3-84A6-4B6D-8B2B-7C4D1F2E9A11}";
    static constexpr wchar_t CRED_UUID_AESKEY[] = L"{F8AB1142-1B9C-4A36-9F55-BA5B08F0A3D9}";
};
