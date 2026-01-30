# === FindMySQLConnector.cmake ===
# Custom module to find MySQL Connector C++ 8.2 (supports debug/release separation)

# Manual override paths – settable in CMake GUI
set(MySQLConnector_INCLUDE_DIR "" CACHE PATH "Path to MySQL Connector C++ include directory")
set(MySQLConnector_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to MySQL Connector C++ debug library directory")
set(MySQLConnector_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to MySQL Connector C++ release library directory")

# === Include directory: must contain mysql/jdbc/mysql_driver.h
find_path(MySQLConnector_INCLUDE_DIR
    NAMES mysql/jdbc/mysql_driver.h
    PATHS ${MySQLConnector_INCLUDE_DIR}
)

# === Debug library (search in main + subfolder 'debug')
find_library(MySQLConnector_DEBUG_LIBRARY
    NAMES mysqlcppconn
    PATHS ${MySQLConnector_DEBUG_LIBRARY_DIR}
    PATH_SUFFIXES . debug
    NO_DEFAULT_PATH
)

# === Release library
find_library(MySQLConnector_RELEASE_LIBRARY
    NAMES mysqlcppconn
    PATHS ${MySQLConnector_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# === Validation
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQLConnector
    REQUIRED_VARS
        MySQLConnector_INCLUDE_DIR
        MySQLConnector_DEBUG_LIBRARY
        MySQLConnector_RELEASE_LIBRARY
)

# === Create imported target if everything found
if(MySQLConnector_FOUND AND NOT TARGET MySQL::Connector)
    add_library(MySQL::Connector UNKNOWN IMPORTED)

    set_target_properties(MySQL::Connector PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${MySQLConnector_DEBUG_LIBRARY}"
        IMPORTED_LOCATION_RELEASE "${MySQLConnector_RELEASE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MySQLConnector_INCLUDE_DIR}"
    )
endif()
