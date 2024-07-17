set(MODULE_NAME "xmf")
set(MODULE_PREFIX "XMF")

set(${MODULE_PREFIX}_SOURCES
    XmfFile.c
    XmfString.c
    XmfTime.c
    XmfImage.c
    XmfBmp.c
    XmfPng.c
    XmfWebM.cpp
    XmfBipBuffer.c
    XmfMkvWriter.c
    XmfNamedPipe.c
    XmfRecorder.c
    XmfMuxer.cpp
    )

set(${MODULE_PREFIX}_HEADERS
    XmfMath.h
    XmfString.h
    XmfFile.h
    XmfTime.h
    XmfImage.h
    XmfBmp.h
    XmfPng.h
    XmfWebM.h
    XmfBipBuffer.h
    XmfMkvWriter.h
    XmfNamedPipe.h
    XmfRecorder.h
    XmfMuxer.h
    )

set(${MODULE_PREFIX}_RESOURCES "")

if(WIN32)
    set(${MODULE_PREFIX}_RESOURCES xmf.rc)

    windows_rc_generate_version_info(
            NAME "Cadeau eXtreme Media Foundation" TYPE "DLL"
            VERSION "${CADEAU_VERSION}"
            FILENAME "xmf.dll"
            VENDOR "${CADEAU_VENDOR}"
            COPYRIGHT "${CADEAU_COPYRIGHT}"
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

    include_directories("${CMAKE_CURRENT_BINARY_DIR}")

    source_group("Resources" FILES ${${MODULE_PREFIX}_RESOURCES})
endif()

add_library(${MODULE_NAME} SHARED
        ${${MODULE_PREFIX}_SOURCES}
        ${${MODULE_PREFIX}_HEADERS}
        ${${MODULE_PREFIX}_RESOURCES}
        $<TARGET_OBJECTS:webm>)

target_link_libraries(${MODULE_NAME} xpp ${CONAN_TARGETS})

if(XMF_COPY_PATH)
    add_custom_command(
        TARGET xmf
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:xmf>
            ${XMF_COPY_PATH}/$<TARGET_FILE_NAME:xmf>)
endif()

if(CADEAU_INSTALL)
    install(TARGETS ${MODULE_NAME}
        EXPORT cadeau-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

    export(TARGETS ${MODULE_NAME}
        APPEND FILE ${PROJECT_BINARY_DIR}/cadeau-targets.cmake)

    install(EXPORT cadeau-targets
            FILE cadeau-targets.cmake
            DESTINATION "${CMAKE_EXPORT_DIR}")
endif()
