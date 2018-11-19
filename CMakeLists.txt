cmake_minimum_required(VERSION 3.0)

project(xpp C)

include(CTest)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(NOT DEFINED XPP_INSTALL)
	set(XPP_INSTALL ON)
endif()

include(CMakePackageConfigHelpers)

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" VERSION_STRINGS)
list(GET VERSION_STRINGS 0 VERSION_STRING)

string(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+" "\\1" XPP_VERSION_MAJOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+" "\\1" XPP_VERSION_MINOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+)" "\\1" XPP_VERSION_PATCH ${VERSION_STRING})
set(XPP_VERSION "${XPP_VERSION_MAJOR}.${XPP_VERSION_MINOR}.${XPP_VERSION_PATCH}")

if(MSVC)
	include(MSVCRuntime)

	if(NOT DEFINED MSVC_RUNTIME)
		set(MSVC_RUNTIME "static")
	endif()

	configure_msvc_runtime()
endif()

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

set(CMAKE_EXPORT_DIR "lib/cmake/xpp")

if(NOT DEFINED WAYK_HOME)
	if(WAYK_HOST STREQUAL Windows)
		set(WAYK_HOME "C:\\wayk")
	else()
		set(WAYK_HOME "/opt/wayk")
	endif()
endif()

if(EXISTS "${WAYK_HOME}" AND NOT DEFINED WITH_WAYK)
	set(WITH_WAYK 1)
	include(WaykSDK)
endif()

if(NOT DEFINED WITH_SIMD)
	set(WITH_SIMD 1)
endif()

# libyuv

if(NOT DEFINED WITH_LIBYUV)
	set(WITH_LIYUV 1)
endif()

find_path(LIBYUV_INCLUDE_DIR
	NAMES libyuv/libyuv.h
	PATH_SUFFIXES include)

if(WIN32)
	find_library(LIBYUV_LIBRARY
		NAMES libyuv.lib
		PATH_SUFFIXES lib)
else()
	find_library(LIBYUV_LIBRARY
		NAMES yuv
		PATH_SUFFIXES lib)
endif()

if(LIBYUV_INCLUDE_DIR AND LIBYUV_LIBRARY)
	set(LIBYUV_FOUND TRUE)
	add_definitions("-DWITH_LIBYUV")
	add_library(libyuv STATIC IMPORTED)
	set_property(TARGET libyuv PROPERTY IMPORTED_LOCATION ${LIBYUV_LIBRARY})
endif()

if(LIBYUV_FOUND)
	message(STATUS "Found libyuv: ${LIBYUV_LIBRARY}")
else()
	message(STATUS "libyuv was not found")
endif()

# halide

if(NOT DEFINED WITH_HALIDE AND HALIDE_LLVM_FOUND)
	set(WITH_HALIDE 1)
endif()

include_directories(include)

add_subdirectory(libxpp)

# installation

if(XPP_INSTALL)
	export(PACKAGE xpp)

	configure_package_config_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/xpp-config.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/xpp-config.cmake"
		INSTALL_DESTINATION "${CMAKE_EXPORT_DIR}")

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/xpp-config-version.cmake"
		VERSION ${XPP_VERSION}
		COMPATIBILITY SameMajorVersion)

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/xpp"
		DESTINATION include
		FILES_MATCHING PATTERN "*.h")

	install(FILES
		${PROJECT_BINARY_DIR}/xpp-config.cmake
		${PROJECT_BINARY_DIR}/xpp-config-version.cmake
		DESTINATION "${CMAKE_EXPORT_DIR}")
endif()

