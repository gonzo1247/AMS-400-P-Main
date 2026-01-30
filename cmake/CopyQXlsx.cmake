function(copy_qxlsx_dlls target)
# fallback if DLL path not explicitly set
if(NOT DEFINED QXlsx_RELEASE_DLL_DIR AND DEFINED QXlsx_RELEASE_LIB)
    get_filename_component(QXlsx_RELEASE_DLL_DIR "${QXlsx_RELEASE_LIB}" DIRECTORY)
endif()
    file(GLOB DEBUG_DLLS "${QXlsx_DEBUG_LIBRARY_DIR}/*.dll")
    file(GLOB RELEASE_DLLS "${QXlsx_RELEASE_LIBRARY_DIR}/*.dll")

    foreach(dll IN LISTS DEBUG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying QXlsx Debug DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Debug
        )
    endforeach()

    foreach(dll IN LISTS RELEASE_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying QXlsx Release DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Release
        )
    endforeach()
endfunction()