#pragma once

#include <cstdint>
#include <optional>

#include "Crypto.h"  // AesKey etc.

class MasterKeyStore
{
   public:
    // Main entry point: load existing or create & store a new one
    static std::optional<Crypto::AesKey> LoadOrCreateFileMasterKey();

   private:
    static constexpr const char* FILE_MASTER_KEY_NAME = "FILE_MASTER_KEY";

    static std::optional<Crypto::AesKey> loadMasterKeyFromDb();
    static bool saveNewMasterKeyToDb(const Crypto::AesKey& masterKey, std::uint32_t version);

    static Crypto::AesKey getProtectorKey();
    static std::vector<std::uint8_t> encryptForDb(const Crypto::AesKey& masterKey);
    static std::optional<Crypto::AesKey> decryptFromDb(const std::vector<std::uint8_t>& blob);
};
