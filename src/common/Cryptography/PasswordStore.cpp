#include "PasswordStore.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincred.h>
#include <filesystem>

#include "BootstrapLogger.h"
#include "Crypto.h"
#include "Logger.h"

#pragma comment(lib, "Advapi32.lib")

bool PasswordStore::SavePassword(const std::string& plainPassword)
{
	// AES-256-GCM: Key, IV, Tag
	Crypto::AesKey aesKey = Crypto::GenerateRandomKey();
	Crypto::ByteArray iv, tag;
	Crypto::ByteArray encrypted = Crypto::EncryptAesGcm(plainPassword, aesKey, iv, tag);

	// --- [1] Composition of: [IV | Encrypted | Tag] ---
	Crypto::ByteArray blob;
	blob.reserve(iv.size() + encrypted.size() + tag.size());
	blob.insert(blob.end(), iv.begin(), iv.end());
	blob.insert(blob.end(), encrypted.begin(), encrypted.end());
	blob.insert(blob.end(), tag.begin(), tag.end());

	// --- [2] Encrypt AES key with DPAPI ---
	std::vector<BYTE> aesKeyEncrypted = Crypto::EncryptWithDPAPI(std::vector<BYTE>(aesKey.begin(), aesKey.end()));

	// --- [3] Store encrypted password in the store ---
	CREDENTIALW cred = {};
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = const_cast<LPWSTR>(CRED_UUID_PASSWORD);
	cred.CredentialBlobSize = static_cast<DWORD>(blob.size());
	cred.CredentialBlob = blob.data();
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cred.UserName = const_cast<LPWSTR>(L"WeWi-System-User");

	if (!::CredWriteW(&cred, 0))
		return false;

	// --- [4] Store encrypted AES key separately ---
	CREDENTIALW keyCred = {};
	keyCred.Type = CRED_TYPE_GENERIC;
	keyCred.TargetName = const_cast<LPWSTR>(CRED_UUID_AESKEY);
	keyCred.CredentialBlobSize = static_cast<DWORD>(aesKeyEncrypted.size());
	keyCred.CredentialBlob = aesKeyEncrypted.data();
	keyCred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	keyCred.UserName = const_cast<LPWSTR>(L"WeWi-System-User");

	return ::CredWriteW(&keyCred, 0) == TRUE;
}

std::optional<std::string> PasswordStore::LoadPassword()
{
	// --- [1] Load password data ---
	CREDENTIALW* pCred = nullptr;
	if (!::CredReadW(CRED_UUID_PASSWORD, CRED_TYPE_GENERIC, 0, &pCred))
		return std::nullopt;

	const BYTE* blob = pCred->CredentialBlob;
	const size_t blobSize = pCred->CredentialBlobSize;

	if (blobSize < Crypto::AES_IV_SIZE + Crypto::AES_TAG_SIZE)
	{
		::CredFree(pCred);
		return std::nullopt;
	}

	// --- [2] Load AES key ---
	CREDENTIALW* pKeyCred = nullptr;
	if (!::CredReadW(CRED_UUID_AESKEY, CRED_TYPE_GENERIC, 0, &pKeyCred))
	{
		::CredFree(pCred);
		return std::nullopt;
	}

	// Decrypt DPAPI
	DATA_BLOB inBlob{ .cbData = pKeyCred->CredentialBlobSize, .pbData = pKeyCred->CredentialBlob };
	DATA_BLOB outBlob{};

	if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob))
	{
		::CredFree(pCred);
		::CredFree(pKeyCred);
		return std::nullopt;
	}

	if (outBlob.cbData != Crypto::AES_KEY_SIZE)
	{
		LocalFree(outBlob.pbData);
		::CredFree(pCred);
		::CredFree(pKeyCred);
		return std::nullopt;
	}

	Crypto::AesKey aesKey;
	std::memcpy(aesKey.data(), outBlob.pbData, Crypto::AES_KEY_SIZE);
	LocalFree(outBlob.pbData);

	// --- [3] IV, Encrypted, Split tag ---
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
		return std::nullopt;
	}
}

bool PasswordStore::MigratePasswordFromBin()
{
	const auto crypto = std::make_unique<Crypto>();

	const auto password = crypto->LoadAndSetMySQLPassword();

	if (!password.empty() && SavePassword(password))
	{

/*
		std::filesystem::remove(keyPath);
		std::filesystem::remove(dataPath);*/
		BootstrapLogger::Log("MySQL password migrated to Windows Credential Store and .bin files removed.");
		return true;
	}

	return false;
}
