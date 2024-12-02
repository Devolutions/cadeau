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
    XmfVpxDecoder.c
    XmfVpxEncoder.c
    XmfVpxImage.c
    XmfVpxPacket.c
    XmfVpxFrame.c
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
    XmfVpxDecoder.h
    XmfVpxEncoder.h
    XmfVpxImage.h
    XmfVpxPacket.h
    XmfVpxFrame.h
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
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )

    include_directories("${CMAKE_CURRENT_BINARY_DIR}")

    source_group("Resources" FILES ${${MODULE_PREFIX}_RESOURCES})
endif()

# Shared library target
add_library(${MODULE_NAME} STATIC
    ${${MODULE_PREFIX}_SOURCES}
    ${${MODULE_PREFIX}_HEADERS}
    ${${MODULE_PREFIX}_RESOURCES}
    $<TARGET_OBJECTS:webm>
)

target_link_libraries(${MODULE_NAME} PRIVATE xpp ${CONAN_TARGETS})


# Set properties for static library
set_target_properties(${MODULE_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# AddressSanitizer settings for Debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(${MODULE_NAME} PUBLIC -fsanitize=address)
    target_link_options(${MODULE_NAME} PUBLIC -fsanitize=address -static-libasan)
endif()

# Custom copy command for xmf.dll
if(XMF_COPY_PATH)
    add_custom_command(
        TARGET ${MODULE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${MODULE_NAME}>
            ${XMF_COPY_PATH}/$<TARGET_FILE_NAME:${MODULE_NAME}>
    )
endif()

# Install targets
if(CADEAU_INSTALL)
    install(TARGETS ${MODULE_NAME}
        EXPORT cadeau-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

    install(EXPORT cadeau-targets
            FILE cadeau-targets.cmake
            DESTINATION "${CMAKE_EXPORT_DIR}")
endif()
