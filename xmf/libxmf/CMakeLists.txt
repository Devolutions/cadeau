set(MODULE_NAME "xmf")
set(MODULE_PREFIX "XMF")

set(${MODULE_PREFIX}_SOURCES
    XmfCom.c
    XmfFile.c
    XmfTime.c
    XmfWebM.cpp
    XmfRecorder.c)

set(${MODULE_PREFIX}_HEADERS
    XmfCom.h
    XmfMath.h
    XmfFile.h
    XmfTime.h
    XmfWebM.h
    XmfRecorder.h)

set(${MODULE_PREFIX}_RESOURCES "")

if(WIN32)
	set(${MODULE_PREFIX}_RESOURCES xmf.rc)

	windows_rc_generate_version_info(
			NAME "XMF" TYPE "DLL"
			VERSION "${XMF_VERSION}"
			FILENAME "xmf.dll"
			VENDOR "${XMF_VENDOR}"
            COPYRIGHT "${XMF_COPYRIGHT}"
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

	include_directories("${CMAKE_CURRENT_BINARY_DIR}")

	source_group("Resources" FILES ${${MODULE_PREFIX}_RESOURCES})
endif()

add_library(${MODULE_NAME} SHARED
		${${MODULE_PREFIX}_SOURCES}
		${${MODULE_PREFIX}_HEADERS}
		${${MODULE_PREFIX}_RESOURCES})

target_link_libraries(${MODULE_NAME} ${CONAN_TARGETS})
