/*
 * Copyright (C) 2023 - 2025 Severin Weitz, Lukas Winter | WeWi-Systems
 *
 * This file is part of the Inventory Management System project,
 * licensed under the GNU Lesser General Public License (LGPL) v3.
 *
 * For more details, see the full license header in main.cpp or
 * InventoryManagementSystem.cpp and the LICENSE.txt file.
 *
 * Author: Severin Weitz, Lukas Winter
 * Date: [07.10.2024]
 */

#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/blowfish.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <fstream>
#include <cstring>
#include <string>
#include <bitset>
#include <random>
#include <iostream>
#include <vector>

class Crypto
{
public:
	Crypto();
	Crypto(const std::string& key);
	~Crypto();

	// Old OpenSSL 1.1.1 - remove in future!
	// ToDo: remove this in future

	bool generateKey(int bits = 2048);

	std::string encryptRSA(const std::string& plaintext) const;
	std::string decryptRSA(const std::string& ciphertext);

	std::string encryptBlowfish(const std::string& plainText);
	std::string decryptBlowfish(const std::string& cipherText);

	std::string removeTrailingNulls(const std::string& str);
	std::string removeCharacters(const std::string& str);
	std::string removeToMoreCharacters(const std::string str, std::uint32_t maxLength);

	std::string EncryptNumberWithRadomValue(std::uint32_t number);
	std::uint32_t DecryptNumberWithRandomValue(const std::string& shiftedString);

	// AES-256 Encryption
	std::vector<unsigned char> EncryptData(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);
	// AES-256 Decryption
	std::vector<unsigned char> DecryptData(const std::vector<unsigned char>& encryptedData, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);

	void SaveKeyAndIV(const std::string& filename, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);
	void LoadKeyAndIV(const std::string& filename, std::vector<unsigned char>& key, std::vector<unsigned char>& iv);
	void GenerateRandomKeyAndIV(std::vector<unsigned char>& key, std::vector<unsigned char>& iv, size_t keySize, size_t ivSize);

	// MySQL Password
	std::string LoadAndSetMySQLPassword();
	void SetMySQLPasswordKeyPath(const std::string& keyPath) { _mySQLKeyFilePath = keyPath; }
	void SetMySQLPasswordFilePath(const std::string& filePath) { _mysqlPFilePath = filePath; }

	// New OpenSSL 3.5.0

	static constexpr size_t AES_KEY_SIZE = 32;       // 256 Bit
	static constexpr size_t AES_IV_SIZE = 12;        // GCM IV Standard
	static constexpr size_t AES_TAG_SIZE = 16;       // GCM Auth Tag

	using ByteArray = std::vector<unsigned char>;
	using AesKey = std::array<unsigned char, AES_KEY_SIZE>;

	static AesKey GenerateRandomKey();
	static ByteArray EncryptAesGcm(const std::string& plainText, const AesKey& key, ByteArray& outIV, ByteArray& outTag);
	static std::string DecryptAesGcm(const ByteArray& ciphertext, const AesKey& key, const ByteArray& iv, const ByteArray& tag);

	// Windows Credential Store DPAPI Encryption
	static std::vector<BYTE> EncryptWithDPAPI(const std::vector<BYTE>& data);

	// Hash function
	std::string GetSHA256Hash(const std::string& input);

    // File
    std::string Sha256File(const std::string& filePath);

private:
	std::vector<unsigned char> generateIV(int size = 16); // default value for a 128-Bit-IV

	// MySQL Password
	void LoadMySQLPasswordPath();
	std::vector<unsigned char> LoadPasswordFromFile();
	std::string DecryptMySQLPassword(const std::vector<unsigned char>& encryptedData, std::string& file);
	

	std::string keyTest = "aK827V62sR21aF71"; // 16 Bytes for AES-128

	std::string charactersToRemove = "\x01\x02\x03\x04\x05\x06\x07\x0F\x1E";

	std::string _mySQLKeyFilePath;
	std::string _mysqlPFilePath;

	RSA* rsaKey;

	// New OpenSSL 3.5.0

	// AES-256 Encryption
	class EVPContextGuard
	{
	public:
		EVPContextGuard()
		{
			ctx = EVP_CIPHER_CTX_new();
			if (!ctx)
				throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
		}

		~EVPContextGuard()
		{
			EVP_CIPHER_CTX_free(ctx);
		}

		EVP_CIPHER_CTX* get() const { return ctx; }

		// non-copyable
		EVPContextGuard(const EVPContextGuard&) = delete;
		EVPContextGuard& operator=(const EVPContextGuard&) = delete;

		// movable
		EVPContextGuard(EVPContextGuard&&) noexcept = default;
		EVPContextGuard& operator=(EVPContextGuard&&) noexcept = default;

	private:
		EVP_CIPHER_CTX* ctx = nullptr;
	};
};

