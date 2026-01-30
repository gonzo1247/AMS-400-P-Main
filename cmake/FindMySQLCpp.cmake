# FindMySQLCpp.cmake
# Finds MySQL Connector/C++ (legacy JDBC API) and optional Connector/C runtime.
# Creates: MySQL::Connector, MySQL::C
# Exposes (Windows, optional): MYSQL_DLL, MYSQL_C_DLL

set(MYSQL_ROOT_DIR "${MYSQL_ROOT_DIR}" CACHE PATH "Root directory of MySQL Connector/C++")

if (NOT MYSQL_ROOT_DIR)
    # project-local fallback, e.g. third_party drop
    set(MYSQL_ROOT_DIR "${CMAKE_SOURCE_DIR}/libs/mysql")
endif()

# Includes
find_path(MYSQL_INCLUDE_DIR mysql/jdbc.h
    HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
    PATH_SUFFIXES include include/mysql
)
set(MYSQL_INCLUDE_DIR "${MYSQL_INCLUDE_DIR}" CACHE PATH "MySQL include directory")

# C++ import libs (.lib / .so)
# (names vary by version/packaging; cover common ones)
find_library(MYSQL_CPP_LIB_RELEASE NAMES
    mysqlcppconn8 mysqlcppconn mysqlcppconn-static
    HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
    PATH_SUFFIXES lib lib64 Release
)
find_library(MYSQL_CPP_LIB_DEBUG NAMES
    mysqlcppconn8d mysqlcppconnd mysqlcppconn
    HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
    PATH_SUFFIXES lib lib64 Debug
)
set(MYSQL_CPP_LIB_RELEASE "${MYSQL_CPP_LIB_RELEASE}" CACHE FILEPATH "Path to mysqlcppconn(.lib) Release")
set(MYSQL_CPP_LIB_DEBUG   "${MYSQL_CPP_LIB_DEBUG}"   CACHE FILEPATH "Path to mysqlcppconn(.lib) Debug")

# Optional: Connector/C (libmysql) — einige Builds benötigen es nicht, andere schon
find_library(MYSQL_C_LIB NAMES libmysql mysqlclient
    HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
    PATH_SUFFIXES lib lib64
)
# DLLs (optional)
if (WIN32)
    find_file(MYSQL_DLL_RELEASE NAMES
        mysqlcppconn8.dll mysqlcppconn.dll
        HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
        PATH_SUFFIXES bin lib Release
    )
    find_file(MYSQL_DLL_DEBUG NAMES
        mysqlcppconn8d.dll mysqlcppconnd.dll mysqlcppconn8.dll mysqlcppconn.dll
        HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
        PATH_SUFFIXES bin lib Debug
    )
    find_file(MYSQL_C_DLL NAMES libmysql.dll
        HINTS "${MYSQL_ROOT_DIR}" $ENV{MYSQL_ROOT_DIR}
        PATH_SUFFIXES bin lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQLCpp REQUIRED_VARS
    MYSQL_INCLUDE_DIR
    MYSQL_CPP_LIB_RELEASE
    MYSQL_CPP_LIB_DEBUG
)

# MySQL::C (optional, only if found)
if (MYSQL_C_LIB)
    add_library(MySQL::C SHARED IMPORTED GLOBAL)
    set_target_properties(MySQL::C PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
    )
    if (WIN32)
        set_target_properties(MySQL::C PROPERTIES IMPORTED_IMPLIB "${MYSQL_C_LIB}")
        if (MYSQL_C_DLL)
            set_target_properties(MySQL::C PROPERTIES IMPORTED_LOCATION "${MYSQL_C_DLL}")
        endif()
    else()
        set_target_properties(MySQL::C PROPERTIES IMPORTED_LOCATION "${MYSQL_C_LIB}")
    endif()
endif()

# MySQL::Connector
add_library(MySQL::Connector SHARED IMPORTED GLOBAL)
set_target_properties(MySQL::Connector PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
)
if (TARGET MySQL::C)
    set_property(TARGET MySQL::Connector APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES MySQL::C)
endif()

if (WIN32)
    set_target_properties(MySQL::Connector PROPERTIES
        IMPORTED_IMPLIB_DEBUG   "${MYSQL_CPP_LIB_DEBUG}"
        IMPORTED_IMPLIB_RELEASE "${MYSQL_CPP_LIB_RELEASE}"
    )
    if (MYSQL_DLL_DEBUG)
        set_target_properties(MySQL::Connector PROPERTIES IMPORTED_LOCATION_DEBUG "${MYSQL_DLL_DEBUG}")
    endif()
    if (MYSQL_DLL_RELEASE)
        set_target_properties(MySQL::Connector PROPERTIES IMPORTED_LOCATION_RELEASE "${MYSQL_DLL_RELEASE}")
    endif()
else()
    set_target_properties(MySQL::Connector PROPERTIES
        IMPORTED_LOCATION "${MYSQL_CPP_LIB_RELEASE}")
endif()

# Helpful cache vars for post-build copy
if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND MYSQL_DLL_DEBUG)
        set(MYSQL_DLL "${MYSQL_DLL_DEBUG}" CACHE FILEPATH "MySQL C++ DLL" FORCE)
    else()
        set(MYSQL_DLL "${MYSQL_DLL_RELEASE}" CACHE FILEPATH "MySQL C++ DLL" FORCE)
    endif()
    set(MYSQL_C_DLL "${MYSQL_C_DLL}" CACHE FILEPATH "MySQL C DLL" FORCE)
endif()

mark_as_advanced(
    MYSQL_ROOT_DIR
    MYSQL_INCLUDE_DIR
    MYSQL_CPP_LIB_RELEASE
    MYSQL_CPP_LIB_DEBUG
    MYSQL_C_LIB
    MYSQL_DLL_RELEASE
    MYSQL_DLL_DEBUG
    MYSQL_C_DLL
)
