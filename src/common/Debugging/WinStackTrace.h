#pragma once

#include <cstddef>

void LogCurrentStackTrace(std::size_t skipFrames = 0, std::size_t maxFrames = 48);
