#pragma once

#include <cstddef>

void WriteExceptionReportToFile(const char* kind, const char* message, std::size_t skipFrames = 0,
                                std::size_t maxFrames = 64);
