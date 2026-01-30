#include "pch.h"
#include "FileCrypto.h"

#include "FileKeyProvider.h"
#include "Logger.h"

namespace FileCrypto
{
    std::optional<std::vector<std::uint8_t>> Encrypt(const std::string& plain)
    {
        if (!FileKeyProvider::IsInitialized())
        {
            LOG_ERROR("FileCrypto::Encrypt: FileKeyProvider not initialized");
            return std::nullopt;
        }

        const Crypto::AesKey& key = FileKeyProvider::GetKey();

        Crypto::ByteArray iv;
        Crypto::ByteArray tag;

        Crypto::ByteArray cipher;
        try
        {
            cipher = Crypto::EncryptAesGcm(plain, key, iv, tag);
        }
        catch (...)
        {
            LOG_ERROR("FileCrypto::Encrypt: EncryptAesGcm threw");
            return std::nullopt;
        }

        std::vector<std::uint8_t> blob;
        blob.reserve(iv.size() + cipher.size() + tag.size());
        blob.insert(blob.end(), iv.begin(), iv.end());
        blob.insert(blob.end(), cipher.begin(), cipher.end());
        blob.insert(blob.end(), tag.begin(), tag.end());

        return blob;
    }

    std::optional<std::string> Decrypt(const std::vector<std::uint8_t>& blob)
    {
        if (!FileKeyProvider::IsInitialized())
        {
            LOG_ERROR("FileCrypto::Decrypt: FileKeyProvider not initialized");
            return std::nullopt;
        }

        if (blob.size() < Crypto::AES_IV_SIZE + Crypto::AES_TAG_SIZE)
        {
            LOG_ERROR("FileCrypto::Decrypt: blob too small");
            return std::nullopt;
        }

        const size_t ivSize = Crypto::AES_IV_SIZE;
        const size_t tagSize = Crypto::AES_TAG_SIZE;
        const size_t cipherSize = blob.size() - ivSize - tagSize;

        const auto* data = blob.data();

        Crypto::ByteArray iv(data, data + ivSize);
        Crypto::ByteArray cipher(data + ivSize, data + ivSize + cipherSize);
        Crypto::ByteArray tag(data + ivSize + cipherSize, data + blob.size());

        const Crypto::AesKey& key = FileKeyProvider::GetKey();

        try
        {
            return Crypto::DecryptAesGcm(cipher, key, iv, tag);
        }
        catch (...)
        {
            LOG_ERROR("FileCrypto::Decrypt: DecryptAesGcm threw");
            return std::nullopt;
        }
    }

}  // namespace FileCrypto
