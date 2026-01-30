#include "pch.h"

#include "NetworkSharePasswordStore.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincred.h>

#include <filesystem>
#include <memory>

#include "Crypto.h"
#include "Logger.h"

#pragma comment(lib, "Advapi32.lib")

bool NetworkSharePasswordStore::SavePassword(const std::string& plainPassword)
{
    // AES-256-GCM: Key, IV, Tag
    Crypto::AesKey aesKey = Crypto::GenerateRandomKey();
    Crypto::ByteArray iv;
    Crypto::ByteArray tag;
    Crypto::ByteArray encrypted = Crypto::EncryptAesGcm(plainPassword, aesKey, iv, tag);

    // [1] Compose blob: [IV | Ciphertext | Tag]
    Crypto::ByteArray blob;
    blob.reserve(iv.size() + encrypted.size() + tag.size());
    blob.insert(blob.end(), iv.begin(), iv.end());
    blob.insert(blob.end(), encrypted.begin(), encrypted.end());
    blob.insert(blob.end(), tag.begin(), tag.end());

    // [2] Encrypt AES key with DPAPI
    std::vector<BYTE> aesKeyEncrypted = Crypto::EncryptWithDPAPI(std::vector<BYTE>(aesKey.begin(), aesKey.end()));

    // [3] Store encrypted password
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(CRED_UUID_PASSWORD);
    cred.CredentialBlobSize = static_cast<DWORD>(blob.size());
    cred.CredentialBlob = blob.data();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = const_cast<LPWSTR>(L"WeWi-System-User");

    if (!::CredWriteW(&cred, 0))
    {
        DWORD err = GetLastError();
        LOG_ERROR("NetworkSharePasswordStore::SavePassword: CredWriteW(password) failed with {}", err);
        return false;
    }

    // [4] Store encrypted AES key
    CREDENTIALW keyCred = {};
    keyCred.Type = CRED_TYPE_GENERIC;
    keyCred.TargetName = const_cast<LPWSTR>(CRED_UUID_AESKEY);
    keyCred.CredentialBlobSize = static_cast<DWORD>(aesKeyEncrypted.size());
    keyCred.CredentialBlob = aesKeyEncrypted.data();
    keyCred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    keyCred.UserName = const_cast<LPWSTR>(L"WeWi-System-User");

    if (!::CredWriteW(&keyCred, 0))
    {
        DWORD err = GetLastError();
        LOG_ERROR("NetworkSharePasswordStore::SavePassword: CredWriteW(aesKey) failed with {}", err);

        // Best effort cleanup if key write fails
        ::CredDeleteW(CRED_UUID_PASSWORD, CRED_TYPE_GENERIC, 0);
        return false;
    }

    return true;
}

std::optional<std::string> NetworkSharePasswordStore::LoadPassword()
{
    // [1] Load password blob
    CREDENTIALW* pCred = nullptr;
    if (!::CredReadW(CRED_UUID_PASSWORD, CRED_TYPE_GENERIC, 0, &pCred))
    {
        return std::nullopt;
    }

    const BYTE* blob = pCred->CredentialBlob;
    const size_t blobSize = pCred->CredentialBlobSize;

    if (blobSize < Crypto::AES_IV_SIZE + Crypto::AES_TAG_SIZE)
    {
        ::CredFree(pCred);
        LOG_WARNING("NetworkSharePasswordStore::LoadPassword: blob too small");
        return std::nullopt;
    }

    // [2] Load encrypted AES key
    CREDENTIALW* pKeyCred = nullptr;
    if (!::CredReadW(CRED_UUID_AESKEY, CRED_TYPE_GENERIC, 0, &pKeyCred))
    {
        ::CredFree(pCred);
        return std::nullopt;
    }

    DATA_BLOB inBlob{.cbData = pKeyCred->CredentialBlobSize, .pbData = pKeyCred->CredentialBlob};

    DATA_BLOB outBlob{};
    if (!::CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob))
    {
        ::CredFree(pCred);
        ::CredFree(pKeyCred);
        LOG_ERROR("NetworkSharePasswordStore::LoadPassword: CryptUnprotectData failed");
        return std::nullopt;
    }

    if (outBlob.cbData != Crypto::AES_KEY_SIZE)
    {
        LocalFree(outBlob.pbData);
        ::CredFree(pCred);
        ::CredFree(pKeyCred);
        LOG_ERROR("NetworkSharePasswordStore::LoadPassword: AES key size mismatch");
        return std::nullopt;
    }

    Crypto::AesKey aesKey;
    std::memcpy(aesKey.data(), outBlob.pbData, Crypto::AES_KEY_SIZE);
    LocalFree(outBlob.pbData);

    // [3] Split IV, ciphertext, tag
    constexpr size_t ivSize = Crypto::AES_IV_SIZE;
    constexpr size_t tagSize = Crypto::AES_TAG_SIZE;
    const size_t cipherSize = blobSize - ivSize - tagSize;

    const Crypto::ByteArray iv(blob, blob + ivSize);
    const Crypto::ByteArray ciphertext(blob + ivSize, blob + ivSize + cipherSize);
    const Crypto::ByteArray tag(blob + ivSize + cipherSize, blob + blobSize);

    ::CredFree(pCred);
    ::CredFree(pKeyCred);

    try
    {
        return Crypto::DecryptAesGcm(ciphertext, aesKey, iv, tag);
    }
    catch (...)
    {
        LOG_ERROR("NetworkSharePasswordStore::LoadPassword: DecryptAesGcm threw");
        return std::nullopt;
    }
}

bool NetworkSharePasswordStore::DeletePassword()
{
    bool ok1 = (::CredDeleteW(CRED_UUID_PASSWORD, CRED_TYPE_GENERIC, 0) == TRUE);
    bool ok2 = (::CredDeleteW(CRED_UUID_AESKEY, CRED_TYPE_GENERIC, 0) == TRUE);

    if (!ok1 || !ok2)
    {
        DWORD err = GetLastError();
        LOG_WARNING("NetworkSharePasswordStore::DeletePassword: CredDeleteW failed with {}", err);
    }

    // If both are already gone, that is fine
    return true;
}
