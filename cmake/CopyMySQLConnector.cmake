function(copy_mysql_connector_dlls target)
# fallback if DLL path not explicitly set
if(NOT DEFINED MySQLConnector_RELEASE_DLL_DIRRARY AND DEFINED MySQLConnector_RELEASE_LIBRARY)
    get_filename_component(MySQLConnector_RELEASE_DLL_DIRRARY "${MySQLConnector_RELEASE_LIBRARY}" DIRECTORY)
endif()
    file(GLOB DEBUG_DLLS "${MySQLConnector_DEBUG_LIBRARY_DIR}/*.dll")
    file(GLOB RELEASE_DLLS "${MySQLConnector_RELEASE_LIBRARY_DIR}/*.dll")

    foreach(dll IN LISTS DEBUG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying MySQL Connector Debug DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Debug
        )
    endforeach()

    foreach(dll IN LISTS RELEASE_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying MySQL Connector Release DLL: ${dll}"
            VERBATIM
            CONFIGURATIONS Release
        )
    endforeach()
endfunction()