#include "pch.h"
#include "MasterKeyStore.h"

#include <cstring>
#include <vector>

#include "ConnectionGuard.h"
#include "DatabaseConnection.h"
#include "Logger.h"

// DB_CRYPTO_KEYS_SELECT_BY_NAME(KeyName)
// DB_CRYPTO_KEYS_INSERT(KeyName, KeyVersion, EncryptedKey, Algorithm, CreatedAt, UpdatedAt)
// DB_CRYPTO_KEYS_UPDATE(KeyVersion, EncryptedKey, Algorithm, UpdatedAt, KeyName)

std::optional<Crypto::AesKey> MasterKeyStore::LoadOrCreateFileMasterKey()
{
    if (auto key = loadMasterKeyFromDb())
    {
        return key;
    }

    LOG_WARNING("MasterKeyStore::LoadOrCreateFileMasterKey: no key in DB, creating new master key");

    Crypto::AesKey masterKey = Crypto::GenerateRandomKey();
    constexpr std::uint32_t version = 1;

    if (!saveNewMasterKeyToDb(masterKey, version))
    {
        LOG_ERROR("MasterKeyStore::LoadOrCreateFileMasterKey: failed to save new master key to DB");
        return std::nullopt;
    }

    return masterKey;
}

std::optional<Crypto::AesKey> MasterKeyStore::loadMasterKeyFromDb()
{
    try
    {
        ConnectionGuardAMS conn(ConnectionType::Sync);
        auto stmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_CS_SELECT_BY_NAME);
        stmt->SetString(0, FILE_MASTER_KEY_NAME);

        auto result = conn->ExecutePreparedSelect(*stmt);
        if (!result.IsValid())
        {
            return std::nullopt;
        }

        auto* fields = result.Fetch();
        if (!fields)
        {
            return std::nullopt;
        }

        const std::string algorithm = fields[0].GetString();           // Algorithm
        const std::vector<std::uint8_t> blob = fields[1].GetBinaryVectorUInt8();  // EncryptedKey

        if (algorithm != "AES-256-GCM")
        {
            LOG_ERROR("MasterKeyStore::loadMasterKeyFromDb: unsupported algorithm {}", algorithm);
            return std::nullopt;
        }

        return decryptFromDb(blob);
    }
    catch (...)
    {
        LOG_ERROR("MasterKeyStore::loadMasterKeyFromDb: exception while loading key from DB");
        return std::nullopt;
    }
}

bool MasterKeyStore::saveNewMasterKeyToDb(const Crypto::AesKey& masterKey, std::uint32_t version)
{
    const std::vector<std::uint8_t> encrypted = encryptForDb(masterKey);
    const std::string algorithm = "AES-256-GCM";

    const std::string now = Util::CurrentDateTimeStringStd();

    try
    {
        ConnectionGuardAMS conn(ConnectionType::Sync);
        {
            auto updateStmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_CS_UPDATE_KEY);
            updateStmt->SetUInt(0, version);
            updateStmt->SetBinary(1, encrypted);
            updateStmt->SetString(2, algorithm);
            updateStmt->SetString(3, now);
            updateStmt->SetString(4, FILE_MASTER_KEY_NAME);

            conn->ExecutePreparedUpdate(*updateStmt);
            if (conn->GetAffectedRows() > 0)
            {
                return true;
            }
        }
        
        {
            auto insertStmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_CS_INSERT_NEW_KEY);
            insertStmt->SetString(0, FILE_MASTER_KEY_NAME);
            insertStmt->SetUInt(1, version);
            insertStmt->SetBinary(2, encrypted);
            insertStmt->SetString(3, algorithm);
            insertStmt->SetString(4, now);
            insertStmt->SetString(5, now);

            conn->ExecutePreparedInsert(*insertStmt);
        }

        return true;
    }
    catch (...)
    {
        LOG_ERROR("MasterKeyStore::saveNewMasterKeyToDb: exception while saving key to DB");
        return false;
    }
}

Crypto::AesKey MasterKeyStore::getProtectorKey()
{
    Crypto::AesKey key{};

    // TODO: choose your own 32 random bytes here
    static constexpr unsigned char RAW_KEY[Crypto::AES_KEY_SIZE] = {
        0xD2, 0xA3, 0x91, 0x7F, 0x4C, 0x1B, 0x8E, 0x23, 0xE4, 0x55, 0x6A, 0xB9, 0x1C, 0x7D, 0x3E, 0x80,
        0x29, 0x4F, 0xAA, 0x13, 0x97, 0xCD, 0x5B, 0xF2, 0x61, 0x38, 0x0D, 0xFE, 0x72, 0x84, 0xB1, 0x09};

    std::memcpy(key.data(), RAW_KEY, Crypto::AES_KEY_SIZE);
    return key;
}

std::vector<std::uint8_t> MasterKeyStore::encryptForDb(const Crypto::AesKey& masterKey)
{
    const Crypto::AesKey protectorKey = getProtectorKey();

    Crypto::ByteArray iv;
    Crypto::ByteArray tag;

    const std::string plain(reinterpret_cast<const char*>(masterKey.data()), Crypto::AES_KEY_SIZE);

    Crypto::ByteArray cipher = Crypto::EncryptAesGcm(plain, protectorKey, iv, tag);

    std::vector<std::uint8_t> blob;
    blob.reserve(iv.size() + cipher.size() + tag.size());
    blob.insert(blob.end(), iv.begin(), iv.end());
    blob.insert(blob.end(), cipher.begin(), cipher.end());
    blob.insert(blob.end(), tag.begin(), tag.end());

    return blob;
}

std::optional<Crypto::AesKey> MasterKeyStore::decryptFromDb(const std::vector<std::uint8_t>& blob)
{
    if (blob.size() < Crypto::AES_IV_SIZE + Crypto::AES_TAG_SIZE)
    {
        LOG_ERROR("MasterKeyStore::decryptFromDb: blob too small");
        return std::nullopt;
    }

    const size_t ivSize = Crypto::AES_IV_SIZE;
    const size_t tagSize = Crypto::AES_TAG_SIZE;
    const size_t cipherSize = blob.size() - ivSize - tagSize;

    const auto* data = blob.data();

    Crypto::ByteArray iv(data, data + ivSize);
    Crypto::ByteArray cipher(data + ivSize, data + ivSize + cipherSize);
    Crypto::ByteArray tag(data + ivSize + cipherSize, data + blob.size());

    const Crypto::AesKey protectorKey = getProtectorKey();

    try
    {
        std::string plain = Crypto::DecryptAesGcm(cipher, protectorKey, iv, tag);
        if (plain.size() != Crypto::AES_KEY_SIZE)
        {
            LOG_ERROR("MasterKeyStore::decryptFromDb: plain size mismatch");
            return std::nullopt;
        }

        Crypto::AesKey masterKey{};
        std::memcpy(masterKey.data(), plain.data(), Crypto::AES_KEY_SIZE);
        return masterKey;
    }
    catch (...)
    {
        LOG_ERROR("MasterKeyStore::decryptFromDb: decryption failed");
        return std::nullopt;
    }
}
