function(copy_kdreports_dlls target)
# fallback if DLL path not explicitly set
if(NOT DEFINED KDReports_RELEASE_MAIN_DLL_DIR AND DEFINED KDReports_RELEASE_MAIN_LIB)
    get_filename_component(KDReports_RELEASE_MAIN_DLL_DIR "${KDReports_RELEASE_MAIN_LIB}" DIRECTORY)
endif()
    file(GLOB DEBUG_DLLS "${KDReports_DEBUG_LIBRARY_DIR}/*.dll")
    file(GLOB RELEASE_DLLS "${KDReports_RELEASE_LIBRARY_DIR}/*.dll")

    foreach(dll IN LISTS DEBUG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying KDReports Debug DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Debug
        )
    endforeach()

    foreach(dll IN LISTS RELEASE_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying KDReports Release DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Release
        )
    endforeach()
endfunction()