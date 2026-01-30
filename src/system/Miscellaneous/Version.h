#pragma once

#include "build_version.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION 999
#endif

#define APP_MAJOR        "0"
#define APP_MINOR        "0"
#define APP_PATCH        "2"

// Build-Status
#ifdef _DEBUG
#define BUILD_STATUS     "Debug Build"
#else
#define BUILD_STATUS     "Release Build"
#endif

#define VERSION_INFO     "Intern Alpha"
#define VERSION_DATE     "30.01.2026"

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define VERSION_NUMBER   APP_MAJOR "." APP_MINOR "." APP_PATCH "." STRINGIFY(BUILD_VERSION)
