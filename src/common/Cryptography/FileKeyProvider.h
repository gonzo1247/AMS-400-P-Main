#pragma once

#include <optional>

#include "Crypto.h"

class FileKeyProvider
{
   public:
    static bool Init();
    static bool IsInitialized();
    static const Crypto::AesKey& GetKey();

   private:
    static std::optional<Crypto::AesKey> _fileMasterKey;
};
