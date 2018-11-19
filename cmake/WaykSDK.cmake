#
# Wayk SDK
#
# WAYK_HOME=/opt/wayk,C:\wayk
# WAYK_SDK=${WAYK_HOME}/sdk
#
# WAYK_PLATFORM=Windows,macOS,Linux,Android,iOS,UWP
# WAYK_ARCH=x86,x64,i386,x86_64,amd64,arm,arm64,armv7,armv7s
# WAYK_HOST=Windows,macOS,Linux
# WAYK_HOST_ARCH=x86,x64,i386,x86_64,amd64
#
# Windows: x86, x64
# macOS: i386, x86_64
# Linux: i386, amd64
#
# iOS: i386, x86_64, armv7, armv7s, arm64
# Android: x86, x86_64, arm, arm64
#

# Detect target platform (Windows, macOS, Linux, Android, iOS, UWP)

if((CMAKE_SYSTEM_NAME MATCHES "WindowsStore") AND (CMAKE_SYSTEM_VERSION MATCHES "10.0"))
	set(WAYK_UWP 1)
	set(WAYK_PLATFORM "UWP")
	add_definitions("-D_UWP")
	set(CMAKE_WINDOWS_VERSION "WIN10")
endif()

if(ANDROID)
	set(WAYK_ANDROID 1)
	set(WAYK_PLATFORM "Android")
elseif(IOS)
	set(WAYK_IOS 1)
	set(WAYK_PLATFORM "iOS")
elseif(APPLE)
	set(WAYK_MACOS 1)
	set(WAYK_PLATFORM "macOS")
elseif(WIN32 AND NOT WAYK_UWP)
	set(WAYK_WIN32 1)
	set(WAYK_WINDOWS 1)
	set(WAYK_PLATFORM "Windows")
elseif(UNIX)
	set(WAYK_LINUX 1)
	set(WAYK_PLATFORM "Linux")
endif()

# Detect target family (Desktop, Mobile)

if(WAYK_WINDOWS OR WAYK_MACOS OR WAYK_LINUX)
	set(WAYK_DESKTOP 1)
	set(WAYK_FAMILY "Desktop")
elseif()
	set(WAYK_MOBILE 1)
	set(WAYK_FAMILY "Mobile")
endif()

# Detect host platform and architecture (Windows, macOS, Linux)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
	set(WAYK_HOST "Windows")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WAYK_HOST_ARCH "x64")
	else()
		set(WAYK_HOST_ARCH "x86")
	endif()
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
	set(WAYK_HOST "macOS")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WAYK_HOST_ARCH "x86_64")
	else()
		set(WAYK_HOST_ARCH "i386")
	endif()
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
	set(WAYK_HOST "Linux")
	execute_process(COMMAND dpkg --print-architecture
		OUTPUT_VARIABLE DPKG_ARCH
  		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(WAYK_HOST_ARCH ${DPKG_ARCH})
endif()

# Find wayk home directory

if(NOT DEFINED WAYK_HOME)
	if($ENV{WAYK_HOME})
		set(WAYK_HOME $ENV{WAYK_HOME})
	else()
		if(WAYK_HOST STREQUAL Windows)
			set(WAYK_HOME "C:\\wayk")
		else()
			set(WAYK_HOME "/opt/wayk")
		endif()
	endif()
endif()

if(NOT EXISTS ${WAYK_HOME})
	message(FATAL_ERROR "The WAYK_HOME (${WAYK_HOME}) could not be found.")
endif()

# Configure SDK and platform variables

set(WAYK_SDK "${WAYK_HOME}/sdk")

if(NOT EXISTS ${WAYK_SDK})
	message(FATAL_ERROR "The WAYK_SDK (${WAYK_SDK}) could not be found.")
endif()

set(WAYK_SDK_ROOT "${WAYK_SDK}/${WAYK_PLATFORM}")
set(WAYK_SDK_HOST "${WAYK_SDK}/${WAYK_HOST}")
set(WAYK_SDK_TOOLS "${WAYK_SDK_HOST}/tools")

set(WAYK_SDK_PACKAGES "mbedtls;jpeg")

if(WAYK_PLATFORM STREQUAL Windows)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WAYK_ARCH "x64")
	else()
		set(WAYK_ARCH "x86")
	endif()
elseif(WAYK_PLATFORM STREQUAL macOS)
	set(WAYK_ARCH_UNIBIN 1)
elseif(WAYK_PLATFORM STREQUAL Linux)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WAYK_ARCH "amd64")
	else()
		set(WAYK_ARCH "i386")
	endif()
elseif(WAYK_PLATFORM STREQUAL iOS)
	set(WAYK_ARCH_UNIBIN 1)
elseif(WAYK_PLATFORM STREQUAL Android)
	set(WAYK_ARCH "${ANDROID_SYSROOT_ABI}")
elseif(WAYK_PLATFORM STREQUAL UWP)

endif()

list(APPEND WAYK_SDK_PACKAGES "libyuv")
list(APPEND WAYK_SDK_PACKAGES "libpng")
list(APPEND WAYK_SDK_PACKAGES "zlib")

list(APPEND WAYK_SDK_PACKAGES "cares")
list(APPEND WAYK_SDK_PACKAGES "curl")
list(APPEND WAYK_SDK_PACKAGES "nng")

# Add SDK tools to the program path

list(APPEND CMAKE_PROGRAM_PATH "${WAYK_SDK_TOOLS}")

# Add SDK packages to the prefix path

foreach(WAYK_SDK_PACKAGE ${WAYK_SDK_PACKAGES})
	if(WAYK_ARCH_UNIBIN)
		list(APPEND CMAKE_PREFIX_PATH "${WAYK_SDK_ROOT}/${WAYK_SDK_PACKAGE}")
	else()
		list(APPEND CMAKE_PREFIX_PATH "${WAYK_SDK_ROOT}/${WAYK_ARCH}/${WAYK_SDK_PACKAGE}")
	endif()
endforeach()

# find LLVM

if(WAYK_HOST STREQUAL Windows)
	set(HALIDE_LLVM_PREFIX "${WAYK_SDK_HOST}/x86/llvm")
elseif(WAYK_HOST STREQUAL macOS)
	set(HALIDE_LLVM_PREFIX "${WAYK_SDK_HOST}/llvm")
elseif(WAYK_HOST STREQUAL Linux)
	set(HALIDE_LLVM_PREFIX "${WAYK_SDK_HOST}/${WAYK_HOST_ARCH}/llvm")
endif()

if(EXISTS "${HALIDE_LLVM_PREFIX}")
	set(HALIDE_LLVM_FOUND 1)
	message(STATUS "Using LLVM: ${HALIDE_LLVM_PREFIX}")
else()
	message(WARNING "LLVM could not be found at ${HALIDE_LLVM_PREFIX}")
endif()

# find Halide

if(WAYK_HOST STREQUAL Linux)
	set(HALIDE_PREFIX "${WAYK_SDK_HOST}/${WAYK_HOST_ARCH}/halide")
else()
	set(HALIDE_PREFIX "${WAYK_SDK_HOST}/halide")
endif()

set(HALIDE_INCLUDE_DIR "${HALIDE_PREFIX}/include")
set(HALIDE_LIBRARY_DIR "${HALIDE_PREFIX}/lib")

