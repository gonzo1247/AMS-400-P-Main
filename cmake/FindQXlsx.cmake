# === FindQXlsx.cmake ===
# Custom Find-Modul für QXlsx mit Qt6 (getrennte Debug/Release Libraries)

set(QXlsx_INCLUDE_DIR "" CACHE PATH "Path to QXlsx header directory")
set(QXlsx_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to QXlsx Debug lib directory")
set(QXlsx_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to QXlsx Release lib directory")

# Header-Verzeichnis finden
find_path(QXlsx_INCLUDE_DIR
    NAMES QXlsx/header/xlsxdocument.h
    PATHS ${QXlsx_INCLUDE_DIR}
)

# Debug-Lib
find_library(QXlsx_DEBUG_LIB
    NAMES QXlsxQt6
    PATHS ${QXlsx_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Release-Lib
find_library(QXlsx_RELEASE_LIB
    NAMES QXlsxQt6
    PATHS ${QXlsx_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Prüfung
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QXlsx
    REQUIRED_VARS
        QXlsx_INCLUDE_DIR
        QXlsx_DEBUG_LIB
        QXlsx_RELEASE_LIB
)

# Target erzeugen
if(QXlsx_FOUND AND NOT TARGET QXlsx::QXlsx)
    add_library(QXlsx::QXlsx UNKNOWN IMPORTED)

    set_target_properties(QXlsx::QXlsx PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${QXlsx_DEBUG_LIB}"
        IMPORTED_LOCATION_RELEASE "${QXlsx_RELEASE_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${QXlsx_INCLUDE_DIR}"
    )
endif()
