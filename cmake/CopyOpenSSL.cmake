function(copy_openssl_dlls target)
# fallback if DLL path not explicitly set
if(NOT DEFINED OpenSSL3_SSL_RELEASE_DLL_DIR AND DEFINED OpenSSL3_SSL_RELEASE_LIB)
    get_filename_component(OpenSSL3_SSL_RELEASE_DLL_DIR "${OpenSSL3_SSL_RELEASE_LIB}" DIRECTORY)
endif()
    file(GLOB DEBUG_DLLS "${OpenSSL3_DEBUG_LIBRARY_DIR}/*.dll")
    file(GLOB RELEASE_DLLS "${OpenSSL3_RELEASE_LIBRARY_DIR}/*.dll")

    foreach(dll IN LISTS DEBUG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying OpenSSL Debug DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Debug
        )
    endforeach()

    foreach(dll IN LISTS RELEASE_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying OpenSSL Release DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Release
        )
    endforeach()
endfunction()