cmake_minimum_required(VERSION 3.24)

project(AssignmentAndMaintenanceSystem LANGUAGES CXX)

option(USE_MARIADB "Use MariaDB Connector/C++ (required)" ON)
option(ENABLE_PROFILING "Enable ScopedTimer profiling logs" OFF)

# ---------------------------------------------------------------
# Custom modules
# ---------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------
# C++ setup
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/MP)
endif()

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)

# ---------------------------------------------------------------
# Qlementine
# ---------------------------------------------------------------
FetchContent_Declare(qlementine
    GIT_REPOSITORY https://github.com/oclero/qlementine.git
    GIT_TAG v1.4.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(qlementine)

# ---------------------------------------------------------------
# Qlementine Icons
# ---------------------------------------------------------------
FetchContent_Declare(qlementine-icons
    GIT_REPOSITORY "https://github.com/oclero/qlementine-icons.git"
    GIT_TAG v1.12.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(qlementine-icons)

# ---------------------------------------------------------------
# Qt
# ---------------------------------------------------------------
find_package(Qt6 6.8.3 REQUIRED COMPONENTS
    Core Gui Widgets Xml Svg Concurrent Charts
    PrintSupport Sql SerialPort Network UiPlugin
    TextToSpeech WebEngineWidgets WebChannel
)

qt_standard_project_setup()

# ---------------------------------------------------------------
# Source setup
# ---------------------------------------------------------------
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE H_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.[hi]pp ${SRC_DIR}/*.h)
list(FILTER H_FILES EXCLUDE REGEX ".*/ui_.*\\.h$")
file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.ui)
file(GLOB_RECURSE RC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.rc)

set(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ams.qrc)
qt_add_resources(PROJECT_RESOURCES ${QRC_FILE})

set(PROJECT_SOURCES
    ${CPP_FILES}
    ${H_FILES}
    ${UI_FILES}
    ${RC_FILES}
    ${PROJECT_RESOURCES}
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ---------------------------------------------------------------
# Profiling
# ---------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<BOOL:${ENABLE_PROFILING}>:ENABLE_PROFILING=1>
    $<$<NOT:$<BOOL:${ENABLE_PROFILING}>>:ENABLE_PROFILING=0>
)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/system/pch/pch.h)

# ---------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------
file(GLOB_RECURSE ALL_INCLUDE_DIRS LIST_DIRECTORIES true ${SRC_DIR}/*)
foreach(dir ${ALL_INCLUDE_DIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# ---------------------------------------------------------------
# Visual Studio filters
# ---------------------------------------------------------------
source_group(TREE ${SRC_DIR} PREFIX "Source Files"  FILES ${CPP_FILES})
source_group(TREE ${SRC_DIR} PREFIX "Header Files"  FILES ${H_FILES})
source_group(TREE ${SRC_DIR} PREFIX "UI Files"      FILES ${UI_FILES})
source_group(TREE ${SRC_DIR} PREFIX "Resource Files" FILES ${RC_FILES})
source_group("Qt Resources" FILES ${QRC_FILE})

# ======================================================================
# MariaDB Connector/C++ (multi-config aware)
# ======================================================================
if (NOT USE_MARIADB)
    message(FATAL_ERROR "This project supports only MariaDB Connector/C++. Set USE_MARIADB=ON.")
endif()

message(STATUS "DB backend: MariaDB Connector/C++")

set(_mariadbcpp_target "")
set(_mariadb_c_target  "")

# 1) Try official packages
find_package(MariaDBConnectorCXX CONFIG QUIET)
if (MariaDBConnectorCXX_FOUND)
    if (TARGET MariaDB::mariadbcpp)
        set(_mariadbcpp_target MariaDB::mariadbcpp)
    elseif (TARGET MariaDB::ConnectorCpp)
        set(_mariadbcpp_target MariaDB::ConnectorCpp)
    endif()
endif()

# 2) Try generic "MariaDB"
if (NOT _mariadbcpp_target)
    find_package(MariaDB QUIET)
    if (MariaDB_FOUND)
        if (TARGET MariaDB::mariadbcpp)
            set(_mariadbcpp_target MariaDB::mariadbcpp)
        elseif (TARGET MariaDB::ConnectorCpp)
            set(_mariadbcpp_target MariaDB::ConnectorCpp)
        endif()
        if (TARGET MariaDB::libmariadb)
            set(_mariadb_c_target MariaDB::libmariadb)
        elseif (TARGET MariaDB::C)
            set(_mariadb_c_target MariaDB::C)
        endif()
    endif()
endif()

# 3) Manual fallback (multi-config)
if (NOT _mariadbcpp_target)
    find_path(MARIADBCPP_INCLUDE_DIR mariadb/conncpp.hpp
              PATHS $ENV{MARIADB_CONNECTOR_CXX_DIR} PATH_SUFFIXES include)

    find_library(MARIADBCPP_LIB_RELEASE NAMES mariadbcpp
                 PATHS $ENV{MARIADB_CONNECTOR_CXX_DIR} PATH_SUFFIXES lib lib64)
    find_library(MARIADBCPP_LIB_DEBUG NAMES mariadbcppd mariadbcpp
                 PATHS $ENV{MARIADB_CONNECTOR_CXX_DIR} PATH_SUFFIXES lib lib64)

    if (MARIADBCPP_INCLUDE_DIR AND (MARIADBCPP_LIB_RELEASE OR MARIADBCPP_LIB_DEBUG))
        add_library(mariadbcpp_import UNKNOWN IMPORTED)
        set_target_properties(mariadbcpp_import PROPERTIES
            IMPORTED_CONFIGURATIONS "Debug;Release"
            INTERFACE_INCLUDE_DIRECTORIES "${MARIADBCPP_INCLUDE_DIR}"
        )
        if (MARIADBCPP_LIB_RELEASE)
            set_target_properties(mariadbcpp_import PROPERTIES
                IMPORTED_LOCATION_RELEASE "${MARIADBCPP_LIB_RELEASE}")
        endif()
        if (MARIADBCPP_LIB_DEBUG)
            set_target_properties(mariadbcpp_import PROPERTIES
                IMPORTED_LOCATION_DEBUG "${MARIADBCPP_LIB_DEBUG}")
        endif()
        set(_mariadbcpp_target mariadbcpp_import)
    endif()
endif()

if (NOT _mariadbcpp_target)
    message(FATAL_ERROR "Could not find MariaDB Connector/C++ (mariadbcpp). "
                        "Install mariadb-connector-cpp or set MARIADB_CONNECTOR_CXX_DIR.")
endif()

# Alias and link
add_library(DB::Connector ALIAS ${_mariadbcpp_target})
target_compile_definitions(${PROJECT_NAME} PRIVATE DB_BACKEND_MARIADB=1)
target_link_libraries(${PROJECT_NAME} PRIVATE DB::Connector)

# Optional: libmariadb (C API) copy
if (NOT _mariadb_c_target)
    find_library(MARIADB_C_LIB_RELEASE NAMES libmariadb
                 PATHS $ENV{MARIADB_CONNECTOR_CXX_DIR} PATH_SUFFIXES lib lib64)
    find_library(MARIADB_C_LIB_DEBUG NAMES libmariadb
                 PATHS $ENV{MARIADB_CONNECTOR_CXX_DIR} PATH_SUFFIXES lib lib64)
    if (MARIADB_C_LIB_RELEASE OR MARIADB_C_LIB_DEBUG)
        add_library(libmariadb_import UNKNOWN IMPORTED)
        set_target_properties(libmariadb_import PROPERTIES
            IMPORTED_CONFIGURATIONS "Debug;Release")
        if (MARIADB_C_LIB_RELEASE)
            set_target_properties(libmariadb_import PROPERTIES
                IMPORTED_LOCATION_RELEASE "${MARIADB_C_LIB_RELEASE}")
        endif()
        if (MARIADB_C_LIB_DEBUG)
            set_target_properties(libmariadb_import PROPERTIES
                IMPORTED_LOCATION_DEBUG "${MARIADB_C_LIB_DEBUG}")
        endif()
        set(_mariadb_c_target libmariadb_import)
    endif()
endif()

# Copy MariaDB DLLs after build
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:${_mariadbcpp_target}>"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM)

    if (_mariadb_c_target)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "$<TARGET_FILE:${_mariadb_c_target}>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM)
    endif()
endif()

# ---------------------------------------------------------------
# OpenSSL
# ---------------------------------------------------------------
find_package(OpenSSL3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto OpenSSL::SSL)

# ---------------------------------------------------------------
# OpenCV
# ---------------------------------------------------------------
find_package(OpenCV490 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenCV::World)

# ---------------------------------------------------------------
# ZXing
# ---------------------------------------------------------------
find_package(ZXing REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZXing::ZXing)

# ---------------------------------------------------------------
# KDReports
# ---------------------------------------------------------------
find_package(KDReports REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE KDReports::KDReports KDReports::KDReportsTestTools)

# ---------------------------------------------------------------
# QXlsx
# ---------------------------------------------------------------
find_package(QXlsx REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx::QXlsx)

# ---------------------------------------------------------------
# Qlementine
# ---------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE qlementine)

# ---------------------------------------------------------------
# Qlementine Icons
# ---------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE qlementine-icons)

# ---------------------------------------------------------------
# LibSSH2
# ---------------------------------------------------------------
find_package(LibSSH2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE LibSSH2::libssh2)

# ---------------------------------------------------------------
# Windows libs
# ---------------------------------------------------------------
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Ws2_32 UxTheme Dwmapi)
endif()

# ---------------------------------------------------------------
# Qt linkage
# ---------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core Qt::Gui Qt::Widgets Qt::Xml Qt::Svg
        Qt::Concurrent Qt::Charts Qt::PrintSupport Qt::Sql
        Qt::SerialPort Qt::Network Qt::UiPlugin Qt::TextToSpeech
        Qt6::WebEngineWidgets Qt6::WebChannel
)

# ---------------------------------------------------------------
# Qt deployment
# ---------------------------------------------------------------
include(cmake/CopyQtDeployment.cmake)
deploy_qt(${PROJECT_NAME} "$<TARGET_FILE:${PROJECT_NAME}>")

# ---------------------------------------------------------------
# Executable flags
# ---------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    AUTOMOC TRUE
    AUTOUIC TRUE
    AUTORCC TRUE
    AUTOGEN_SOURCE_GROUP "Generated Files"
    AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/generated"
)

# ---------------------------------------------------------------
# Optional release DLL copy
# ---------------------------------------------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    include(cmake/CopyUtils.cmake)
    include(cmake/CopyOpenSSL.cmake)
    include(cmake/CopyOpenCV.cmake)
    include(cmake/CopyZXing.cmake)
    include(cmake/CopyKDReports.cmake)
    include(cmake/CopyQXlsx.cmake)

    copy_openssl_dlls(${PROJECT_NAME})
    copy_opencv_dlls(${PROJECT_NAME})
    copy_zxing_dlls(${PROJECT_NAME})
    copy_kdreports_dlls(${PROJECT_NAME})
    copy_qxlsx_dlls(${PROJECT_NAME})
endif()

# ---------------------------------------------------------------
# clang-format
# ---------------------------------------------------------------
find_program(CLANG_FORMAT_BIN NAMES clang-format)
if (CLANG_FORMAT_BIN)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h"
        "${SRC_DIR}/*.cxx" "${SRC_DIR}/*.hpp")
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT_BIN} -i --style=file ${ALL_CXX_SOURCE_FILES}
        COMMENT "Running clang-format on source files")
else()
    message(WARNING "clang-format not found — skipping format target.")
endif()

# ---------------------------------------------------------------
# Visual Studio integration
# ---------------------------------------------------------------
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if (MSVC)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running build number update..."
        COMMAND cmd /c "${CMAKE_SOURCE_DIR}\\buildversion\\update_build_number.bat"
    )
endif()
