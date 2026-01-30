set(MySQLCAPI_INCLUDE_DIR "" CACHE PATH "Path to MySQL C API include dir (contains mysql.h)")
set(MySQLCAPI_LIBRARY_DIR "" CACHE PATH "Path to MySQL C API lib dir (contains libmysql.lib)")

find_path(MySQLCAPI_INCLUDE_DIR
    NAMES mysql.h
    PATHS ${MySQLCAPI_INCLUDE_DIR}
)

find_library(MySQLCAPI_LIBRARY
    NAMES libmysql
    PATHS ${MySQLCAPI_LIBRARY_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQLCAPI
    REQUIRED_VARS
        MySQLCAPI_INCLUDE_DIR
        MySQLCAPI_LIBRARY
)

if(MySQLCAPI_FOUND AND NOT TARGET MySQL::CAPI)
    add_library(MySQL::CAPI UNKNOWN IMPORTED)
    set_target_properties(MySQL::CAPI PROPERTIES
        IMPORTED_LOCATION "${MySQLCAPI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MySQLCAPI_INCLUDE_DIR}"
    )
endif()
