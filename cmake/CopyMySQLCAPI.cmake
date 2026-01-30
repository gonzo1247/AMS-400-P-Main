function(copy_mysql_capi_dlls TARGET)
# fallback if DLL path not explicitly set
if(NOT DEFINED MySQLCAPI_DLL_DIRRARY AND DEFINED MySQLCAPI_LIBRARY)
    get_filename_component(MySQLCAPI_DLL_DIRRARY "${MySQLCAPI_LIBRARY}" DIRECTORY)
endif()
    if(WIN32)
        # Akzeptiere sowohl _LIBRARY als auch _LIBRARY_DIR
        if(MySQLCAPI_LIBRARY)
            set(MYSQLCAPI_LIB_PATH "${MySQLCAPI_LIBRARY}")
        elseif(MySQLCAPI_LIBRARY_DIR)
            set(MYSQLCAPI_LIB_PATH "${MySQLCAPI_LIBRARY_DIR}")
        else()
            message(WARNING "MySQLCAPI_LIBRARY[_DIR] not set. Skipping MySQL C API DLL copy.")
            return()
        endif()

        if(NOT EXISTS "${MYSQLCAPI_LIB_PATH}")
            message(WARNING "MySQLCAPI path '${MYSQLCAPI_LIB_PATH}' does not exist. Skipping DLL copy.")
            return()
        endif()

        file(GLOB CAPI_DLLS "${MYSQLCAPI_LIB_PATH}/*.dll")

        foreach(dll_file IN LISTS CAPI_DLLS)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll_file}"
                $<TARGET_FILE_DIR:${TARGET}>
                COMMENT "Copying MySQL C API DLL: ${dll_file}"
            )
        endforeach()
    endif()
endfunction()