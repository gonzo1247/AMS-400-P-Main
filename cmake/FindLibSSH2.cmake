# === FindLibSSH2.cmake ===
# Custom finder for libssh2 (Windows & Unix), with Debug/Release support and imported target.
#
# Exposes:
#   LibSSH2_FOUND
#   LibSSH2_INCLUDE_DIR
#   LibSSH2_DEBUG_LIB
#   LibSSH2_RELEASE_LIB
#   target: LibSSH2::libssh2  (IMPORTED, with DEBUG/RELEASE locations)
#
# Cache variables for manual override (Windows-friendly):
set(LibSSH2_INCLUDE_DIR "" CACHE PATH "Path to libssh2 include directory")
set(LibSSH2_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to Debug lib directory (e.g. vc16/lib)")
set(LibSSH2_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to Release lib directory")
set(LIBSSH2_USE_STATIC OFF CACHE BOOL "Prefer static libssh2 library if available")

# --- Optional deps ---
# Try pkg-config first on non-Windows for hints
if(NOT WIN32)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBSSH2 QUIET libssh2)
  endif()
endif()

# --- Include path ---
# Typical headers: libssh2.h under include/ or include/libssh2/
find_path(LibSSH2_INCLUDE_DIR
  NAMES libssh2.h libssh2/libssh2.h
  PATHS
    ${LibSSH2_INCLUDE_DIR}
    ${PC_LIBSSH2_INCLUDEDIR}
    ${PC_LIBSSH2_INCLUDE_DIRS}
  PATH_SUFFIXES .
  NO_DEFAULT_PATH
)

# Fallback to default search paths if not provided
if(NOT LibSSH2_INCLUDE_DIR)
  find_path(LibSSH2_INCLUDE_DIR
    NAMES libssh2.h libssh2/libssh2.h
    PATH_SUFFIXES include
  )
endif()

# --- Library names ---
# Windows often: libssh2.lib / libssh2d.lib
# Unix: libssh2.so/.a (name "ssh2" or "libssh2")
set(_LIBSSH2_NAMES_RELEASE libssh2 ssh2)
set(_LIBSSH2_NAMES_DEBUG   libssh2d libssh2_debug ssh2d)

# Helper to choose STATIC or SHARED if both exist
set(_CMAKE_FIND_LIBRARY_SUFFIXES_SAVED "${CMAKE_FIND_LIBRARY_SUFFIXES}")
if(LIBSSH2_USE_STATIC)
  if(WIN32)
    # Prefer .lib static first, then shared
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a .dll.a .dll)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so .dylib)
  endif()
endif()

# --- Find release lib ---
find_library(LibSSH2_RELEASE_LIB
  NAMES ${_LIBSSH2_NAMES_RELEASE}
  PATHS
    ${LibSSH2_RELEASE_LIBRARY_DIR}
    ${PC_LIBSSH2_LIBDIR}
    ${PC_LIBSSH2_LIBRARY_DIRS}
  NO_DEFAULT_PATH
)

if(NOT LibSSH2_RELEASE_LIB)
  find_library(LibSSH2_RELEASE_LIB
    NAMES ${_LIBSSH2_NAMES_RELEASE}
    PATH_SUFFIXES lib lib64
  )
endif()

# --- Find debug lib ---
find_library(LibSSH2_DEBUG_LIB
  NAMES ${_LIBSSH2_NAMES_DEBUG} ${_LIBSSH2_NAMES_RELEASE} # fallback to release name if no *d
  PATHS
    ${LibSSH2_DEBUG_LIBRARY_DIR}
    ${PC_LIBSSH2_LIBDIR}
    ${PC_LIBSSH2_LIBRARY_DIRS}
  NO_DEFAULT_PATH
)

if(NOT LibSSH2_DEBUG_LIB)
  find_library(LibSSH2_DEBUG_LIB
    NAMES ${_LIBSSH2_NAMES_DEBUG} ${_LIBSSH2_NAMES_RELEASE}
    PATH_SUFFIXES lib lib64
  )
endif()

# Restore suffixes
set(CMAKE_FIND_LIBRARY_SUFFIXES "${_CMAKE_FIND_LIBRARY_SUFFIXES_SAVED}")

# If only one config was found, reuse it for the other
if(LibSSH2_RELEASE_LIB AND NOT LibSSH2_DEBUG_LIB)
  set(LibSSH2_DEBUG_LIB "${LibSSH2_RELEASE_LIB}")
endif()
if(LibSSH2_DEBUG_LIB AND NOT LibSSH2_RELEASE_LIB)
  set(LibSSH2_RELEASE_LIB "${LibSSH2_DEBUG_LIB}")
endif()

# --- Optional deps: Zlib & OpenSSL (libssh2 often depends on them) ---
# We don't make them REQUIRED to avoid breaking; if found, we link them on the interface.
find_package(ZLIB QUIET)
find_package(OpenSSL QUIET) # Provides OpenSSL::SSL and OpenSSL::Crypto

# --- Evaluate result ---
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSSH2
  REQUIRED_VARS LibSSH2_INCLUDE_DIR LibSSH2_RELEASE_LIB LibSSH2_DEBUG_LIB
  FAIL_MESSAGE "libssh2 not found. Please set LibSSH2_INCLUDE_DIR and *LIBRARY_DIR caches."
)

if(LibSSH2_FOUND AND NOT TARGET LibSSH2::libssh2)
  add_library(LibSSH2::libssh2 UNKNOWN IMPORTED)

  set_target_properties(LibSSH2::libssh2 PROPERTIES
    IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
    IMPORTED_LOCATION_DEBUG   "${LibSSH2_DEBUG_LIB}"
    IMPORTED_LOCATION_RELEASE "${LibSSH2_RELEASE_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${LibSSH2_INCLUDE_DIR}"
  )

  # Link deps if available
  if(TARGET ZLIB::ZLIB)
    target_link_libraries(LibSSH2::libssh2 INTERFACE ZLIB::ZLIB)
  endif()

  if(TARGET OpenSSL::SSL)
    target_link_libraries(LibSSH2::libssh2 INTERFACE OpenSSL::SSL)
  endif()
  if(TARGET OpenSSL::Crypto)
    target_link_libraries(LibSSH2::libssh2 INTERFACE OpenSSL::Crypto)
  endif()

  # Windows: optionally define LIBSSH2_WIN32 to satisfy some builds (rarely needed)
  if(WIN32)
    target_compile_definitions(LibSSH2::libssh2 INTERFACE LIBSSH2_WIN32)
  endif()
endif()
