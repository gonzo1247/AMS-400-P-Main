function(deploy_qt TARGET_NAME TARGET_FILE_PATH)
    if(NOT WIN32)
        return()
    endif()

    get_filename_component(TARGET_DIR "${TARGET_FILE_PATH}" DIRECTORY)
    get_filename_component(QT_BIN_DIR "${Qt6Core_DIR}/../../../bin" ABSOLUTE)
    set(WINDEPLOYQT_EXE "${QT_BIN_DIR}/windeployqt.exe")

    if(NOT EXISTS "${WINDEPLOYQT_EXE}")
        message(WARNING "windeployqt.exe not found: ${WINDEPLOYQT_EXE}")
        return()
    endif()

    # Erzeuge korrektes Konfigurationsflag
    set(QT_DEPLOY_CONFIG_FLAG "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(QT_DEPLOY_CONFIG_FLAG "--debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(QT_DEPLOY_CONFIG_FLAG "--release")
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND "${WINDEPLOYQT_EXE}"
                "$<TARGET_FILE:${TARGET_NAME}>"
                --no-opengl-sw
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-patchqt
                ${QT_DEPLOY_CONFIG_FLAG}
        COMMENT "Running windeployqt on ${TARGET_NAME} for ${CMAKE_BUILD_TYPE}"
        VERBATIM
    )
endfunction()