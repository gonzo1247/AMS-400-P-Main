# === FindOpenCV490.cmake ===
# Custom CMake finder for OpenCV 4.9.0 using opencv_world490.lib

# Manuell konfigurierbare Einträge
set(OpenCV490_INCLUDE_DIR "" CACHE PATH "Path to OpenCV 4.9.0 include directory")
set(OpenCV490_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to Debug lib directory (e.g. vc16/lib)")
set(OpenCV490_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to Release lib directory")

# Header-Verzeichnis suchen
find_path(OpenCV490_INCLUDE_DIR
    NAMES opencv2/opencv.hpp
    PATHS ${OpenCV490_INCLUDE_DIR}
)

# Debug-Lib (opencv_world490.lib)
find_library(OpenCV490_DEBUG_LIB
    NAMES opencv_world490
    PATHS ${OpenCV490_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Release-Lib
find_library(OpenCV490_RELEASE_LIB
    NAMES opencv_world490
    PATHS ${OpenCV490_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Validierung
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenCV490
    REQUIRED_VARS
        OpenCV490_INCLUDE_DIR
        OpenCV490_DEBUG_LIB
        OpenCV490_RELEASE_LIB
)

# Target erzeugen
if(OpenCV490_FOUND AND NOT TARGET OpenCV::World)
    add_library(OpenCV::World UNKNOWN IMPORTED)

    set_target_properties(OpenCV::World PROPERTIES
        IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
        IMPORTED_LOCATION_DEBUG "${OpenCV490_DEBUG_LIB}"
        IMPORTED_LOCATION_RELEASE "${OpenCV490_RELEASE_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenCV490_INCLUDE_DIR}"
    )
endif()
