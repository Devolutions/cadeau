cmake_minimum_required(VERSION 3.0)

project(xpp C)

include(CTest)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(conan)

# generate conan profile name
if(ANDROID)
	set(CONAN_PROFILE_OS "android")
	set(CONAN_PROFILE_ARCH "${ANDROID_SYSROOT_ABI}")
elseif(IOS)
	set(CONAN_PROFILE_OS "ios")
	set(CONAN_PROFILE_ARCH "universal")
	set(CONAN_PROFILE_ARCHS "${CMAKE_OSX_ARCHITECTURES}")
elseif(APPLE)
	set(CONAN_PROFILE_OS "macos")
	set(CONAN_PROFILE_ARCH "x86_64")
elseif(WIN32)
	set(CONAN_PROFILE_OS "windows")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CONAN_PROFILE_ARCH "x86_64")
	else()
		set(CONAN_PROFILE_ARCH "x86")
	endif()
elseif(UNIX)
	set(CONAN_PROFILE_OS "linux")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CONAN_PROFILE_ARCH "x86_64")
	else()
		set(CONAN_PROFILE_ARCH "x86")
	endif()
endif()

set(CONAN_PROFILE "${CONAN_PROFILE_OS}-${CONAN_PROFILE_ARCH}")

# detect build host os/cpu architecture
if(CMAKE_HOST_APPLE)
    set(CONAN_BUILD_HOST "Macos")
    set(CONAN_BUILD_ARCH "x86_64")
elseif(CMAKE_HOST_WIN32)
    set(CONAN_BUILD_HOST "Windows")
    if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(CONAN_BUILD_ARCH "x86_64")
    else()
        set(CONAN_BUILD_ARCH "x86")
    endif()
elseif(CMAKE_HOST_UNIX)
    set(CONAN_BUILD_HOST "Linux")
    execute_process(
    COMMAND dpkg-architecture -qDEB_HOST_ARCH
        OUTPUT_VARIABLE CMD_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(CMD_OUTPUT STREQUAL "i386")
        set(CONAN_BUILD_ARCH "x86")
    else()
        set(CONAN_BUILD_ARCH "x86_64")
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

conan_check()

conan_cmake_install(
	CONANFILE conanfile.txt
	PROFILE ${CONAN_PROFILE}
	GENERATORS cmake
	UPDATE
	SETTINGS "os_build=${CONAN_BUILD_HOST};arch_build=${CONAN_BUILD_ARCH};build_type=${CMAKE_BUILD_TYPE}"
)

set(CONAN_DISABLE_CHECK_COMPILER ON)
conan_load_buildinfo()
conan_basic_setup(TARGETS NO_OUTPUT_DIRS)

if(NOT DEFINED CONAN_OUTPUT_DIRS)
	if(CMAKE_GENERATOR STREQUAL "Ninja")
		set(CONAN_OUTPUT_DIRS ON)
	else()
		set(CONAN_OUTPUT_DIRS OFF)
	endif()
endif()

if(CONAN_OUTPUT_DIRS)
	conan_output_dirs_setup()
endif()

if(NOT DEFINED XPP_INSTALL)
	set(XPP_INSTALL ON)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

# clang+llvm

set(HALIDE_LLVM_PREFIX "${CONAN_CLANG-LLVM_ROOT}")

if(EXISTS "${HALIDE_LLVM_PREFIX}")
	set(HALIDE_LLVM_FOUND 1)
	message(STATUS "Using LLVM: ${HALIDE_LLVM_PREFIX}")
else()
	message(WARNING "LLVM could not be found at ${HALIDE_LLVM_PREFIX}")
endif()

# halide

set(HALIDE_INCLUDE_DIR "${CONAN_HALIDE_ROOT}/include")
set(HALIDE_LIBRARY_DIR "${CONAN_HALIDE_ROOT}/lib")

if(NOT DEFINED WITH_HALIDE AND HALIDE_LLVM_FOUND)
	set(WITH_HALIDE 1)
endif()

add_definitions(-DXPP_EXPORTS)

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
