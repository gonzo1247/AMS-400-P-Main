/*
Copyright 2023 - 2024 by Severin
And TrinityCore - https://github.com/TrinityCore
*/

#pragma once


#define WUW_PLATFORM_WINDOWS 0
#define WUW_PLATFORM_UNIX    1
#define WUW_PLATFORM_APPLE   2

#if defined( _WIN32 )
#  define WUW_PLATFORM WUW_PLATFORM_WINDOWS
#elif defined( __APPLE__ )
#  define WUW_PLATFORM WUW_PLATFORM_APPLE
#else
#  define WUW_PLATFORM WUW_PLATFORM_UNIX
#endif

#define WUW_COMPILER_MICROSOFT 0
#define WUW_COMPILER_GNU       1
#define WUW_COMPILER_INTEL     2

#ifdef _MSC_VER
#  define WUW_COMPILER WUW_COMPILER_MICROSOFT
#elif defined( __INTEL_COMPILER )
#  define WUW_COMPILER WUW_COMPILER_INTEL
#elif defined( __GNUC__ )
#  define WUW_COMPILER WUW_COMPILER_GNU
#  define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#  error "FATAL ERROR: Unknown compiler."
#endif