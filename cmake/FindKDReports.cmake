# === FindKDReports.cmake ===
# Custom find module for KDReports with Qt6 and Debug/Release separation

set(KDReports_INCLUDE_DIR "" CACHE PATH "Path to KDReports header directory")
set(KDReports_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to KDReports Debug lib directory")
set(KDReports_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to KDReports Release lib directory")

# Includes
find_path(KDReports_INCLUDE_DIR
    NAMES KDReport.h
    PATHS ${KDReports_INCLUDE_DIR}
)

# Debug Libraries
find_library(KDReports_DEBUG_MAIN_LIB
    NAMES kdreports-qt6
    PATHS ${KDReports_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(KDReports_DEBUG_TOOLS_LIB
    NAMES kdreporttesttools
    PATHS ${KDReports_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Release Libraries
find_library(KDReports_RELEASE_MAIN_LIB
    NAMES kdreports-qt6
    PATHS ${KDReports_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(KDReports_RELEASE_TOOLS_LIB
    NAMES kdreporttesttools
    PATHS ${KDReports_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Prüfung
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KDReports
    REQUIRED_VARS
        KDReports_INCLUDE_DIR
        KDReports_DEBUG_MAIN_LIB
        KDReports_DEBUG_TOOLS_LIB
        KDReports_RELEASE_MAIN_LIB
        KDReports_RELEASE_TOOLS_LIB
)

# Main Target
if(KDReports_FOUND AND NOT TARGET KDReports::KDReports)
    add_library(KDReports::KDReports UNKNOWN IMPORTED)
    set_target_properties(KDReports::KDReports PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${KDReports_DEBUG_MAIN_LIB}"
        IMPORTED_LOCATION_RELEASE "${KDReports_RELEASE_MAIN_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${KDReports_INCLUDE_DIR}"
    )
endif()

# Tools Target
if(KDReports_FOUND AND NOT TARGET KDReports::KDReportsTestTools)
    add_library(KDReports::KDReportsTestTools UNKNOWN IMPORTED)
    set_target_properties(KDReports::KDReportsTestTools PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${KDReports_DEBUG_TOOLS_LIB}"
        IMPORTED_LOCATION_RELEASE "${KDReports_RELEASE_TOOLS_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${KDReports_INCLUDE_DIR}"
    )
endif()