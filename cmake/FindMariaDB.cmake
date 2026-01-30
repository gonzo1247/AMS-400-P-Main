# FindMariaDB.cmake
# Finds MariaDB Connector/C++ (mariadbcpp) and Connector/C (libmariadb).
# Creates imported targets:
#   MariaDB::Connector   (C++ connector)
#   MariaDB::C           (C connector)
#
# Cache vars (override as needed) — symmetric to FindMySQLCpp.cmake:
#   MARIADB_ROOT_DIR
#   MARIADB_INCLUDE_DIR
#   MARIADB_CPP_LIB_RELEASE
#   MARIADB_CPP_LIB_DEBUG
#   MARIADB_C_LIB
#   MARIADB_DLL_RELEASE
#   MARIADB_DLL_DEBUG
#   MARIADB_C_DLL
#
# Helper vars for post-build copy (Windows):
#   MARIADB_DLL           -> chosen C++ DLL (debug or release)
#   MARIADB_C_DLL         -> C DLL

# Root directory (MSI install or project-local drop)
set(MARIADB_ROOT_DIR "${MARIADB_ROOT_DIR}" CACHE PATH "Root directory of MariaDB Connector/C++")
if (NOT MARIADB_ROOT_DIR)
    set(MARIADB_ROOT_DIR "${CMAKE_SOURCE_DIR}/libs/mariadb")
endif()

# Includes
find_path(MARIADB_INCLUDE_DIR mariadb/conncpp.hpp
    HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
    PATH_SUFFIXES include include/mariadb
)
set(MARIADB_INCLUDE_DIR "${MARIADB_INCLUDE_DIR}" CACHE PATH "MariaDB include directory")

# C++ import libraries (.lib / .so)
find_library(MARIADB_CPP_LIB_RELEASE NAMES mariadbcpp
    HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
    PATH_SUFFIXES lib lib64 Release
)
find_library(MARIADB_CPP_LIB_DEBUG NAMES mariadbcppd mariadbcpp
    HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
    PATH_SUFFIXES lib lib64 Debug
)
set(MARIADB_CPP_LIB_RELEASE "${MARIADB_CPP_LIB_RELEASE}" CACHE FILEPATH "Path to mariadbcpp.lib (Release)")
set(MARIADB_CPP_LIB_DEBUG   "${MARIADB_CPP_LIB_DEBUG}"   CACHE FILEPATH "Path to mariadbcppd.lib (Debug)")

# C import library (.lib / .so)
find_library(MARIADB_C_LIB NAMES libmariadb mariadb
    HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
    PATH_SUFFIXES lib lib64
)
set(MARIADB_C_LIB "${MARIADB_C_LIB}" CACHE FILEPATH "Path to libmariadb import/shared library")

# Runtime DLLs (Windows)
if (WIN32)
    find_file(MARIADB_DLL_RELEASE NAMES mariadbcpp.dll
        HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
        PATH_SUFFIXES bin lib Release
    )
    find_file(MARIADB_DLL_DEBUG NAMES mariadbcppd.dll mariadbcpp.dll
        HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
        PATH_SUFFIXES bin lib Debug
    )
    find_file(MARIADB_C_DLL NAMES libmariadb.dll libmariadb-3.dll
        HINTS "${MARIADB_ROOT_DIR}" $ENV{MARIADB_ROOT_DIR}
        PATH_SUFFIXES bin lib
    )
    set(MARIADB_DLL_RELEASE "${MARIADB_DLL_RELEASE}" CACHE FILEPATH "MariaDB C++ DLL (Release)")
    set(MARIADB_DLL_DEBUG   "${MARIADB_DLL_DEBUG}"   CACHE FILEPATH "MariaDB C++ DLL (Debug)")
    set(MARIADB_C_DLL       "${MARIADB_C_DLL}"       CACHE FILEPATH "MariaDB C DLL")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MariaDB REQUIRED_VARS
    MARIADB_INCLUDE_DIR
    MARIADB_CPP_LIB_RELEASE
    MARIADB_CPP_LIB_DEBUG
    MARIADB_C_LIB
)

# Imported target: MariaDB::C
add_library(MariaDB::C SHARED IMPORTED GLOBAL)
set_target_properties(MariaDB::C PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${MARIADB_INCLUDE_DIR}"
)
if (WIN32)
    # On Windows: .lib is import lib, .dll is runtime
    set_target_properties(MariaDB::C PROPERTIES
        IMPORTED_IMPLIB "${MARIADB_C_LIB}"
    )
    if (MARIADB_C_DLL)
        set_target_properties(MariaDB::C PROPERTIES
            IMPORTED_LOCATION "${MARIADB_C_DLL}"
        )
    endif()
else()
    # On non-Windows the found library is typically the .so directly
    set_target_properties(MariaDB::C PROPERTIES
        IMPORTED_LOCATION "${MARIADB_C_LIB}"
    )
endif()

# Imported target: MariaDB::Connector
add_library(MariaDB::Connector SHARED IMPORTED GLOBAL)
set_target_properties(MariaDB::Connector PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    INTERFACE_INCLUDE_DIRECTORIES "${MARIADB_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES MariaDB::C
)
if (WIN32)
    set_target_properties(MariaDB::Connector PROPERTIES
        IMPORTED_IMPLIB_DEBUG   "${MARIADB_CPP_LIB_DEBUG}"
        IMPORTED_IMPLIB_RELEASE "${MARIADB_CPP_LIB_RELEASE}"
    )
    if (MARIADB_DLL_DEBUG)
        set_target_properties(MariaDB::Connector PROPERTIES
            IMPORTED_LOCATION_DEBUG "${MARIADB_DLL_DEBUG}"
        )
    endif()
    if (MARIADB_DLL_RELEASE)
        set_target_properties(MariaDB::Connector PROPERTIES
            IMPORTED_LOCATION_RELEASE "${MARIADB_DLL_RELEASE}"
        )
    endif()
else()
    set_target_properties(MariaDB::Connector PROPERTIES
        IMPORTED_LOCATION "${MARIADB_CPP_LIB_RELEASE}"
    )
endif()

# Helper vars for post-build copy (single var like MySQL finder)
if (WIN32)
    # If multiconfig, this is a best-effort default; adjust in your copy step if needed.
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND MARIADB_DLL_DEBUG)
        set(MARIADB_DLL "${MARIADB_DLL_DEBUG}" CACHE FILEPATH "MariaDB C++ DLL (chosen)" FORCE)
    else()
        set(MARIADB_DLL "${MARIADB_DLL_RELEASE}" CACHE FILEPATH "MariaDB C++ DLL (chosen)" FORCE)
    endif()
endif()

mark_as_advanced(
    MARIADB_ROOT_DIR
    MARIADB_INCLUDE_DIR
    MARIADB_CPP_LIB_RELEASE
    MARIADB_CPP_LIB_DEBUG
    MARIADB_C_LIB
    MARIADB_DLL_RELEASE
    MARIADB_DLL_DEBUG
    MARIADB_C_DLL
)
