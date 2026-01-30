# === FindZXing.cmake ===
# ZXing-C++ finder mit Debug/Release-Unterstützung

# Manuell konfigurierbare Pfade
set(ZXing_INCLUDE_DIRS "" CACHE PATH "Pfad(e) zu ZXing Headern (mehrere erlaubt, durch ; trennen)")
set(ZXing_DEBUG_LIBRARY_DIR "" CACHE PATH "Pfad zu ZXing Debug lib (mit ZXing.lib)")
set(ZXing_RELEASE_LIBRARY_DIR "" CACHE PATH "Pfad zu ZXing Release lib (mit ZXing.lib)")

# Library-Dateien finden
find_library(ZXing_DEBUG_LIB
    NAMES ZXing
    PATHS ${ZXing_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(ZXing_RELEASE_LIB
    NAMES ZXing
    PATHS ${ZXing_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Validation
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZXing
    REQUIRED_VARS
        ZXing_INCLUDE_DIRS
        ZXing_DEBUG_LIB
        ZXing_RELEASE_LIB
)

# Target anlegen
if(ZXing_FOUND AND NOT TARGET ZXing::ZXing)
    add_library(ZXing::ZXing UNKNOWN IMPORTED)

    set_target_properties(ZXing::ZXing PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${ZXing_DEBUG_LIB}"
        IMPORTED_LOCATION_RELEASE "${ZXing_RELEASE_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZXing_INCLUDE_DIRS}"
    )
endif()
