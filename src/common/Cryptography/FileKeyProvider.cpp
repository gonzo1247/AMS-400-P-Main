#include "pch.h"
#include "FileKeyProvider.h"

#include "Logger.h"
#include "MasterKeyStore.h"

std::optional<Crypto::AesKey> FileKeyProvider::_fileMasterKey{};

bool FileKeyProvider::Init()
{
    _fileMasterKey = MasterKeyStore::LoadOrCreateFileMasterKey();

    if (!_fileMasterKey.has_value())
    {
        LOG_ERROR("FileKeyProvider::Init: failed to load or create file master key");
        return false;
    }

    LOG_INFO("FileKeyProvider::Init: file master key initialized");
    return true;
}

bool FileKeyProvider::IsInitialized() { return _fileMasterKey.has_value(); }

const Crypto::AesKey& FileKeyProvider::GetKey()
{
    // Assumes Init() was called and succeeded
    return *_fileMasterKey;
}
