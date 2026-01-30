# === FindOpenSSL3.cmake ===
# Custom find module for OpenSSL 3.x with Debug/Release separation

set(OpenSSL3_INCLUDE_DIR "" CACHE PATH "Path to OpenSSL 3.x headers (should contain openssl/ssl.h)")
set(OpenSSL3_DEBUG_LIBRARY_DIR "" CACHE PATH "Path to Debug libs (containing libssl.lib & libcrypto.lib)")
set(OpenSSL3_RELEASE_LIBRARY_DIR "" CACHE PATH "Path to Release libs (containing libssl.lib & libcrypto.lib)")

# Header check
find_path(OpenSSL3_INCLUDE_DIR
    NAMES openssl/ssl.h
    PATHS ${OpenSSL3_INCLUDE_DIR}
)

# Debug libraries
find_library(OpenSSL3_SSL_DEBUG_LIB
    NAMES libssl
    PATHS ${OpenSSL3_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(OpenSSL3_CRYPTO_DEBUG_LIB
    NAMES libcrypto
    PATHS ${OpenSSL3_DEBUG_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Release libraries
find_library(OpenSSL3_SSL_RELEASE_LIB
    NAMES libssl
    PATHS ${OpenSSL3_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(OpenSSL3_CRYPTO_RELEASE_LIB
    NAMES libcrypto
    PATHS ${OpenSSL3_RELEASE_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

# Validation
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL3
    REQUIRED_VARS
        OpenSSL3_INCLUDE_DIR
        OpenSSL3_SSL_DEBUG_LIB
        OpenSSL3_CRYPTO_DEBUG_LIB
        OpenSSL3_SSL_RELEASE_LIB
        OpenSSL3_CRYPTO_RELEASE_LIB
)

# Targets
if(OpenSSL3_FOUND)
    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_DEBUG "${OpenSSL3_CRYPTO_DEBUG_LIB}"
            IMPORTED_LOCATION_RELEASE "${OpenSSL3_CRYPTO_RELEASE_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${OpenSSL3_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_DEBUG "${OpenSSL3_SSL_DEBUG_LIB}"
            IMPORTED_LOCATION_RELEASE "${OpenSSL3_SSL_RELEASE_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${OpenSSL3_INCLUDE_DIR}"
        )
    endif()
endif()