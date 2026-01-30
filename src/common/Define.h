/*
Copyright 2023 - 2024 by Severin
And TrinityCore - https://github.com/TrinityCore
*/

#pragma once

#include "CompilerDefs.h"

#if WUW_COMPILER == WUW_COMPILER_GNU
#  if !defined(__STDC_FORMAT_MACROS)
#    define __STDC_FORMAT_MACROS
#  endif
#  if !defined(__STDC_CONSTANT_MACROS)
#    define __STDC_CONSTANT_MACROS
#  endif
#  if !defined(_GLIBCXX_USE_NANOSLEEP)
#    define _GLIBCXX_USE_NANOSLEEP
#  endif
#  if defined(HELGRIND)
#    include <valgrind/helgrind.h>
#    undef _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE
#    undef _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER
#    define _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(A) ANNOTATE_HAPPENS_BEFORE(A)
#    define _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(A)  ANNOTATE_HAPPENS_AFTER(A)
#  endif
#  if defined(VALGRIND)
#    include <valgrind/memcheck.h>
#  endif
#endif

#include <cstddef>
#include <cinttypes>
#include <climits>

#define WUW_LITTLEENDIAN 0
#define WUW_BIGENDIAN    1

#if !defined(WUW_ENDIAN)
#  if defined (BOOST_BIG_ENDIAN)
#    define WUW_ENDIAN WUW_BIGENDIAN
#  else
#    define WUW_ENDIAN WUW_LITTLEENDIAN
#  endif
#endif

#if WUW_PLATFORM == WUW_PLATFORM_WINDOWS
#  define WUW_PATH_MAX 260
#else // WUW_PLATFORM != WUW_PLATFORM_WINDOWS
#  define WUW_PATH_MAX PATH_MAX
#endif // WUW_PLATFORM

#if !defined(COREDEBUG)
#  define WUW_INLINE inline
#else //COREDEBUG
#  if !defined(WUW_DEBUG)
#    define WUW_DEBUG
#  endif //WUW_DEBUG
#  define WUW_INLINE
#endif //!COREDEBUG

#if WUW_COMPILER == WUW_COMPILER_GNU
#  define ATTR_PRINTF(F, V) __attribute__ ((__format__ (__printf__, F, V)))
#else //WUW_COMPILER != WUW_COMPILER_GNU
#  define ATTR_PRINTF(F, V)
#endif //WUW_COMPILER == WUW_COMPILER_GNU

#ifdef WUW_API_USE_DYNAMIC_LINKING
#  if WUW_COMPILER == WUW_COMPILER_MICROSOFT
#    define TC_API_EXPORT __declspec(dllexport)
#    define TC_API_IMPORT __declspec(dllimport)
#  elif WUW_COMPILER == WUW_COMPILER_GNU
#    define TC_API_EXPORT __attribute__((visibility("default")))
#    define TC_API_IMPORT
#  else
#    error compiler not supported!
#  endif
#else
#  define TC_API_EXPORT
#  define TC_API_IMPORT
#endif

#ifdef WUW_API_EXPORT_COMMON
#  define TC_COMMON_API TC_API_EXPORT
#else
#  define TC_COMMON_API TC_API_IMPORT
#endif

#ifdef WUW_API_EXPORT_PROTO
#  define TC_PROTO_API TC_API_EXPORT
#else
#  define TC_PROTO_API TC_API_IMPORT
#endif

#ifdef WUW_API_EXPORT_DATABASE
#  define TC_DATABASE_API TC_API_EXPORT
#else
#  define TC_DATABASE_API TC_API_IMPORT
#endif

#ifdef WUW_API_EXPORT_SHARED
#  define TC_SHARED_API TC_API_EXPORT
#else
#  define TC_SHARED_API TC_API_IMPORT
#endif

#ifdef WUW_API_EXPORT_GAME
#  define TC_GAME_API TC_API_EXPORT
#else
#  define TC_GAME_API TC_API_IMPORT
#endif

#define UI64FMTD "%" PRIu64
#define UI64LIT(N) UINT64_C(N)

#define SI64FMTD "%" PRId64
#define SI64LIT(N) INT64_C(N)

#define SZFMTD "%" PRIuPTR

#define STRING_VIEW_FMT "%.*s"
#define STRING_VIEW_FMT_ARG(str) static_cast<int>((str).length()), (str).data()

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

enum DBCFormer
{
	FT_STRING = 's',                                        // LocalizedString*
	FT_STRING_NOT_LOCALIZED = 'S',                          // char*
	FT_FLOAT = 'f',                                         // float
	FT_INT = 'i',                                           // uint32
	FT_BYTE = 'b',                                          // uint8
	FT_SHORT = 'h',                                         // uint16
	FT_LONG = 'l'                                           // uint64
};

template <typename E>
constexpr std::underlying_type_t<E> to_underlying(E e) noexcept
{
    static_assert(std::is_enum_v<E>);
#if __cpp_lib_to_underlying >= 202102L
    return std::to_underlying(e);
#else
    return static_cast<std::underlying_type_t<E> >(e);
#endif
}
