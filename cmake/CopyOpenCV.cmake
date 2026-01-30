function(copy_opencv_dlls target)
# fallback if DLL path not explicitly set
if(NOT DEFINED OpenCV490_DLL_DIR AND DEFINED OpenCV490_RELEASE_LIBRARY_DIR)
    get_filename_component(OpenCV490_DLL_DIR "${OpenCV490_RELEASE_LIBRARY_DIR}" DIRECTORY)
endif()
    file(GLOB DEBUG_DLLS "${OpenCV490_DEBUG_LIBRARY_DIR}/*.dll")
    file(GLOB RELEASE_DLLS "${OpenCV490_RELEASE_LIBRARY_DIR}/*.dll")

    foreach(dll IN LISTS DEBUG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying OpenCV Debug DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Debug
        )
    endforeach()

    foreach(dll IN LISTS RELEASE_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying OpenCV Release DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Release
        )
    endforeach()
endfunction()