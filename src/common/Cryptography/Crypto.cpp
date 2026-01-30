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

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Crypto.h"

#include <filesystem>

#include "BootstrapLogger.h"
#include "Logger.h"

#pragma comment(lib, "Crypt32.lib")

Crypto::Crypto() : rsaKey(nullptr)
{
	generateKey();
}

Crypto::~Crypto()
{
	if (rsaKey) 
	{
		RSA_free(rsaKey);
		rsaKey = nullptr;
	}
}

bool Crypto::generateKey(int bits /*= 2048*/)
{
	rsaKey = RSA_generate_key(bits, RSA_F4, nullptr, nullptr);
	return rsaKey != nullptr;
}

std::string Crypto::encryptRSA(const std::string& plaintext) const
{
	if (!rsaKey)
		return "";

	int rsaSize = RSA_size(rsaKey);
	std::vector<unsigned char> ciphertext(rsaSize);

	size_t plaintextLength = plaintext.length();
	int result;

	if (plaintextLength <= static_cast<size_t>(std::numeric_limits<int>::max()))
		result = RSA_public_encrypt(static_cast<int>(plaintextLength),
			reinterpret_cast<const unsigned char*>(plaintext.c_str()),
			ciphertext.data(), rsaKey, RSA_PKCS1_PADDING);
	else
		result = -1;

	if (result == -1)
	{
		ERR_print_errors_fp(stderr);
		return "";
	}

	return std::string(reinterpret_cast<char*>(ciphertext.data()), result);
}

std::string Crypto::decryptRSA(const std::string& ciphertext)
{
	if (!rsaKey)
		return "";

	int rsaSize = RSA_size(rsaKey);
	std::vector<unsigned char> plaintext(rsaSize);

	size_t ciphertextLength = ciphertext.length();
	int result;

	if (ciphertextLength <= static_cast<size_t>(std::numeric_limits<int>::max())) 
		result = RSA_private_decrypt(static_cast<int>(ciphertextLength),
			reinterpret_cast<const unsigned char*>(ciphertext.c_str()),
			plaintext.data(), rsaKey, RSA_PKCS1_PADDING);
	else
		result = -1;

	if (result == -1)
	{
		ERR_print_errors_fp(stderr);
		return "";
	}

	return std::string(reinterpret_cast<char*>(plaintext.data()), result);
}

std::string Crypto::encryptBlowfish(const std::string& plainText)
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);

	EVP_EncryptInit_ex(ctx, EVP_bf_cbc(), nullptr,
		reinterpret_cast<const unsigned char*>(keyTest.c_str()), nullptr);

	std::string cipherText(plainText.size() + EVP_CIPHER_block_size(EVP_bf_cbc()), '\0');

	int len = 0;
	EVP_EncryptUpdate(ctx, const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(cipherText.data())), &len,
		reinterpret_cast<const unsigned char*>(plainText.c_str()), static_cast<int>(plainText.size()));

	int finalLen = 0;
	EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&cipherText[len]), &finalLen);

	EVP_CIPHER_CTX_free(ctx);

	return cipherText;
}

std::string Crypto::decryptBlowfish(const std::string& cipherText)
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);

	EVP_DecryptInit_ex(ctx, EVP_bf_cbc(), nullptr,
		reinterpret_cast<const unsigned char*>(keyTest.c_str()), nullptr);

	std::string recoveredText(cipherText.size(), '\0');

	int len = 0;
	EVP_DecryptUpdate(ctx, const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(recoveredText.data())), &len,
		reinterpret_cast<const unsigned char*>(cipherText.c_str()), static_cast<int>(cipherText.size()));

	int finalLen = 0;
	EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&recoveredText[len]), &finalLen);

	EVP_CIPHER_CTX_free(ctx);

	return recoveredText;
}

std::string Crypto::removeTrailingNulls(const std::string& str)
{
	size_t nullPos = str.find('\0');
	if (nullPos != std::string::npos)
		return str.substr(0, nullPos);

	return str;
}

std::string Crypto::removeCharacters(const std::string& str)
{
	std::string result = str;
	for (char ch : charactersToRemove)
		result.erase(std::remove(result.begin(), result.end(), ch), result.end());

	return result;
}

std::string Crypto::removeToMoreCharacters(const std::string str, std::uint32_t maxLength)
{
	if (str.length() >= maxLength)
	{
		std::string returnString = str.substr(0, maxLength);
		return returnString;
	}

	return str;
}

std::string Crypto::EncryptNumberWithRadomValue(std::uint32_t number)
{
	// Generate random number
	std::random_device rd;
	std::mt19937 gen(rd());
    std::uniform_int_distribution<std::uint32_t> dist(0, 31); // Only use 0 to 31 so as not to exceed 32 bits
    std::uint32_t randomValue = dist(gen);

	// Convert number into a string
	std::bitset<32> shiftedBits(number);

	// Shift number to the left
	shiftedBits <<= randomValue;

	// Append the random number to the end of the string
	std::string shiftedString = shiftedBits.to_string() + '|' + std::to_string(randomValue);

	return shiftedString;
}

std::uint32_t Crypto::DecryptNumberWithRandomValue(const std::string& shiftedString)
{
	// Find the position of the separator
	std::size_t separatorPos = shiftedString.find_last_of('|');
	if (separatorPos == std::string::npos) 
	{
		// Separator not found, handle the error
		std::cerr << "Error: Separator not found in the string." << std::endl;
		return 0;
	}

	// Extract the original bits (without the separator) and convert to uint32_t
	std::bitset<32> shiftedBits(shiftedString.substr(0, separatorPos));
	std::uint32_t originalNumber = shiftedBits.to_ulong();

	// Extract the random value from the string
	std::uint32_t randomValue = std::stoi(shiftedString.substr(separatorPos + 1));

	// Shift the original number back to the right
	shiftedBits >>= randomValue;
	originalNumber = shiftedBits.to_ulong();

	return originalNumber;
}

std::vector<unsigned char> Crypto::EncryptData(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv)
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	std::vector<unsigned char> encryptedData(data.size() + EVP_MAX_BLOCK_LENGTH);

	// Initialization of the encryption process
	EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data());

	// Encrypting the data
	int encryptedLength;
	EVP_EncryptUpdate(ctx, encryptedData.data(), &encryptedLength, data.data(), static_cast<int>(data.size()));

	// Completing the encryption process
	int finalLength;
	EVP_EncryptFinal_ex(ctx, encryptedData.data() + encryptedLength, &finalLength);

	EVP_CIPHER_CTX_free(ctx);

	// Adjust the size of the encrypted text
	encryptedData.resize(encryptedLength + finalLength);

	return encryptedData;
}

std::vector<unsigned char> Crypto::DecryptData(const std::vector<unsigned char>& encryptedData, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv)
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	std::vector<unsigned char> decryptedData(encryptedData.size());

	// Initialization of the encryption process
	EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data());

	// Decrypting the data
	int decryptedLength;
	EVP_DecryptUpdate(ctx, decryptedData.data(), &decryptedLength, encryptedData.data(), static_cast<int>(encryptedData.size()));

	// Completing the decryption process
	int finalLength;
	EVP_DecryptFinal_ex(ctx, decryptedData.data() + decryptedLength, &finalLength);

	EVP_CIPHER_CTX_free(ctx);

	// Adjust the size of the decrypted text
	decryptedData.resize(decryptedLength + finalLength);

	return decryptedData;
}

void Crypto::SaveKeyAndIV(const std::string& filename, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv)
{
	std::ofstream file(filename, std::ios::binary);
	file.write(reinterpret_cast<const char*>(key.data()), key.size());
	file.write(reinterpret_cast<const char*>(iv.data()), iv.size());
	file.close();
}

void Crypto::LoadKeyAndIV(const std::string& filename, std::vector<unsigned char>& key, std::vector<unsigned char>& iv)
{
	std::ifstream file(filename, std::ios::binary);
	key.clear();
	iv.clear();

	size_t keySize = 32;  // Example: AES-256
	size_t ivSize = 16;   // Example: AES-256

	key.resize(keySize);
	iv.resize(ivSize);

	file.read(reinterpret_cast<char*>(key.data()), keySize);
	file.read(reinterpret_cast<char*>(iv.data()), ivSize);

	file.close();
}

std::vector<unsigned char> Crypto::generateIV(int size /*= 16*/)
{
	// Seed for random number generation
	std::random_device rd;
	std::mt19937 gen(rd());

	// Distribution for bytes (0 to 255)
	std::uniform_int_distribution<> dis(0, 255);

	// create vector for iv
	std::vector<unsigned char> iv(size);

	// Fill the vector with random bytes
	for (int i = 0; i < size; ++i)
		iv[i] = static_cast<unsigned char>(dis(gen));

	return iv;
}

void Crypto::GenerateRandomKeyAndIV(std::vector<unsigned char>& key, std::vector<unsigned char>& iv, size_t keySize, size_t ivSize)
{
	key.resize(keySize);
	iv.resize(ivSize);

	// Generate random bytes for the key and the IV
	RAND_bytes(key.data(), static_cast<int>(keySize));
	RAND_bytes(iv.data(), static_cast<int>(ivSize));
}

std::string Crypto::LoadAndSetMySQLPassword()
{
	LoadMySQLPasswordPath();
	const auto encryptedData = LoadPasswordFromFile();
	return DecryptMySQLPassword(encryptedData, _mySQLKeyFilePath);
}

Crypto::AesKey Crypto::GenerateRandomKey()
{
	AesKey key{};
	if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1)
		throw std::runtime_error("Failed to generate random AES key");
	return key;
}

Crypto::ByteArray Crypto::EncryptAesGcm(const std::string& plainText, const AesKey& key, ByteArray& outIV, ByteArray& outTag)
{
	outIV.resize(AES_IV_SIZE);
	if (RAND_bytes(outIV.data(), static_cast<int>(outIV.size())) != 1)
		throw std::runtime_error("Failed to generate IV");

	const EVPContextGuard ctx;

	if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
		throw std::runtime_error("EVP_EncryptInit_ex (algorithm) failed");

	if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), outIV.data()) != 1)
		throw std::runtime_error("EVP_EncryptInit_ex (key+iv) failed");

	ByteArray ciphertext(plainText.size());
	int len = 0;
	if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len,
		reinterpret_cast<const unsigned char*>(plainText.data()),
		static_cast<int>(plainText.size())) != 1)
		throw std::runtime_error("EVP_EncryptUpdate failed");

	int totalLen = len;

	if (EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len) != 1)
		throw std::runtime_error("EVP_EncryptFinal_ex failed");

	totalLen += len;
	ciphertext.resize(totalLen);

	outTag.resize(AES_TAG_SIZE);
	if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, outTag.data()) != 1)
		throw std::runtime_error("Failed to get GCM tag");

	return ciphertext;
}

std::string Crypto::DecryptAesGcm(const ByteArray& ciphertext, const AesKey& key, const ByteArray& iv, const ByteArray& tag)
{
	if (iv.size() != AES_IV_SIZE)
		throw std::runtime_error("Invalid IV size");

	if (tag.size() != AES_TAG_SIZE)
		throw std::runtime_error("Invalid tag size");

	const EVPContextGuard ctx;

	if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
		throw std::runtime_error("EVP_DecryptInit_ex (algorithm) failed");

	if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1)
		throw std::runtime_error("EVP_DecryptInit_ex (key+iv) failed");

	std::vector<unsigned char> decrypted(ciphertext.size());
	int len = 0;

	if (EVP_DecryptUpdate(ctx.get(), decrypted.data(), &len,
		ciphertext.data(), static_cast<int>(ciphertext.size())) != 1)
		throw std::runtime_error("EVP_DecryptUpdate failed");

	int totalLen = len;

	if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, AES_TAG_SIZE,
		const_cast<unsigned char*>(tag.data())) != 1)
		throw std::runtime_error("EVP_CTRL_GCM_SET_TAG failed");

	// Auth tag is verified during final step
	if (EVP_DecryptFinal_ex(ctx.get(), decrypted.data() + len, &len) != 1)
		throw std::runtime_error("EVP_DecryptFinal_ex failed: authentication failed");

	totalLen += len;
	decrypted.resize(totalLen);

	return { reinterpret_cast<char*>(decrypted.data()), decrypted.size() };
}

std::vector<BYTE> Crypto::EncryptWithDPAPI(const std::vector<BYTE>& data)
{
	DATA_BLOB inBlob{ .cbData = static_cast<DWORD>(data.size()),
				  .pbData = const_cast<BYTE*>(data.data()) };

	DATA_BLOB outBlob{};

	if (!CryptProtectData(&inBlob, L"AES Key", nullptr, nullptr, nullptr,
		CRYPTPROTECT_LOCAL_MACHINE, &outBlob))
	{
		throw std::runtime_error("Failed to protect data with DPAPI");
	}

	std::vector<BYTE> encrypted(outBlob.pbData, outBlob.pbData + outBlob.cbData);
	LocalFree(outBlob.pbData);
	return encrypted;
}

std::string Crypto::GetSHA256Hash(const std::string& input)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];

	if (!SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash))
		throw std::runtime_error("SHA256 hashing failed");

	std::ostringstream oss;
	for (unsigned char byte : hash)
		oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);

	return oss.str();
}

std::string Crypto::Sha256File(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Sha256File: unable to open file: " + filePath);
    }

    const EVP_MD* md = EVP_sha256();
    if (!md)
    {
        throw std::runtime_error("Sha256File: EVP_sha256() failed");
    }

    EVP_MD_CTX* rawCtx = EVP_MD_CTX_new();
    if (!rawCtx)
    {
        throw std::runtime_error("Sha256File: EVP_MD_CTX_new() failed");
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(rawCtx, &EVP_MD_CTX_free);

    if (EVP_DigestInit_ex(ctx.get(), md, nullptr) != 1)
    {
        throw std::runtime_error("Sha256File: EVP_DigestInit_ex() failed");
    }

    std::vector<unsigned char> buffer(8192);

    while (file.good())
    {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize bytesRead = file.gcount();

        if (bytesRead > 0)
        {
            if (EVP_DigestUpdate(ctx.get(), buffer.data(), static_cast<size_t>(bytesRead)) != 1)
            {
                throw std::runtime_error("Sha256File: EVP_DigestUpdate() failed");
            }
        }
    }

    unsigned int digestLen = static_cast<unsigned int>(EVP_MD_get_size(md));
    if (digestLen == 0)
    {
        throw std::runtime_error("Sha256File: EVP_MD_get_size() returned 0");
    }

    std::vector<unsigned char> digest(digestLen);

    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestLen) != 1)
    {
        throw std::runtime_error("Sha256File: EVP_DigestFinal_ex() failed");
    }

    static constexpr char hex[] = "0123456789abcdef";

    std::string result;
    result.reserve(digestLen * 2);

    for (unsigned int i = 0; i < digestLen; ++i)
    {
        unsigned char b = digest[i];
        result.push_back(hex[(b >> 4) & 0x0F]);
        result.push_back(hex[b & 0x0F]);
    }

    return result;
}

void Crypto::LoadMySQLPasswordPath()
{
	std::filesystem::path programDataPath = std::filesystem::path(std::getenv("PROGRAMDATA")) / "Lager-und-Bestellverwaltung";
	std::filesystem::create_directories(programDataPath);

	_mySQLKeyFilePath = programDataPath.string() + "\\MKFI.bin";

	_mysqlPFilePath = programDataPath.string() + "\\MPFI.bin";
}

std::vector<unsigned char> Crypto::LoadPasswordFromFile()
{
	std::vector<unsigned char> loadData;
	std::ifstream file(_mysqlPFilePath, std::ios::binary);

	if (file.is_open())
	{
		std::size_t dataLoadSize;
		file.read(reinterpret_cast<char*>(&dataLoadSize), sizeof(dataLoadSize));

		loadData.resize(dataLoadSize);
		file.read(reinterpret_cast<char*>(loadData.data()), dataLoadSize);

		file.close();		
	}
	else
	{
		BootstrapLogger::Log("Can not load Password Data in path {}", _mysqlPFilePath);
	}

	return loadData;
}

std::string Crypto::DecryptMySQLPassword(const std::vector<unsigned char>& encryptedData, std::string& file)
{
	std::vector<unsigned char> keyL, ivL;
	LoadKeyAndIV(file, keyL, ivL);

	std::vector<unsigned char> decryptedDataLoaded = DecryptData(encryptedData, keyL, ivL);

	const auto endIt = std::ranges::find(decryptedDataLoaded, '\0');
	return std::string(decryptedDataLoaded.begin(), endIt);
}
