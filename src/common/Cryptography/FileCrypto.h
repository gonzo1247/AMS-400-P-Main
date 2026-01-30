#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Crypto.h"

namespace FileCrypto
{
    // Layout: [IV | Cipher | Tag]
    std::optional<std::vector<std::uint8_t>> Encrypt(const std::string& plain);
    std::optional<std::string> Decrypt(const std::vector<std::uint8_t>& blob);
}  // namespace FileCrypto
