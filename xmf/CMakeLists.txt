cmake_minimum_required(VERSION 3.15)

project(xmf C CXX)

include(CTest)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT DEFINED MSVC_RUNTIME)
    set(MSVC_RUNTIME "static")
endif()

if(MSVC AND (MSVC_RUNTIME STREQUAL "static"))
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake")

option(USE_CONAN "Use conan package manager" ON)
option(XMF_INSTALL "Generate cmake installation script" ON)

if(MSVC)
	include(WindowsRC)
endif()

include(ConanHelper)
include(CMakePackageConfigHelpers)

file(STRINGS "${CMAKE_SOURCE_DIR}/../VERSION" VERSION_STRINGS)
list(GET VERSION_STRINGS 0 VERSION_STRING)

string(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+.[0-9]+" "\\1" XMF_VERSION_MAJOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+.[0-9]+" "\\1" XMF_VERSION_MINOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+).[0-9]+" "\\1" XMF_VERSION_PATCH ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.[0-9]+.[0-9]+.([0-9]+)" "\\1" XMF_VERSION_BUILD ${VERSION_STRING})
set(XMF_VERSION "${XMF_VERSION_MAJOR}.${XMF_VERSION_MINOR}.${XMF_VERSION_PATCH}.${XMF_VERSION_BUILD}")

set(XMF_VENDOR "Devolutions Inc.")
set(XMF_COPYRIGHT "Copyright 2022, ${XMF_VENDOR}")

if(WIN32)
	set(C_FLAGS "")
	set(C_FLAGS "${C_FLAGS} -D_UNICODE")
	set(C_FLAGS "${C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
	set(C_FLAGS "${C_FLAGS} -DWIN32_LEAN_AND_MEAN")
	set(C_FLAGS "${C_FLAGS} -D_WINSOCK_DEPRECATED_NO_WARNINGS")
	set(C_FLAGS "${C_FLAGS} -DWINVER=0x0601 -D_WIN32_WINNT=0x0601")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS}")
endif()

if(CMAKE_GENERATOR MATCHES "Ninja")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

set(CMAKE_EXPORT_DIR "lib/cmake/xmf")

add_definitions(-DXPP_EXPORTS)
add_definitions(-DXMF_EXPORTS)

include_directories(include)

add_definitions(-DWITH_LIBVPX)
add_definitions(-DWITH_LIBWEBM)

add_subdirectory(libxmf)

if(XMF_INSTALL)
	install(TARGETS ${MODULE_NAME}
		EXPORT libxmf-targets
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib)

	export(TARGETS ${MODULE_NAME}
		APPEND FILE ${PROJECT_BINARY_DIR}/libxmf-targets.cmake)
endif()
