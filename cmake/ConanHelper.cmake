
# generate conan profile name
if(ANDROID)
    set(CONAN_PROFILE_OS "android")
    set(CONAN_PROFILE_ARCH "${ANDROID_SYSROOT_ABI}")
elseif(IOS)
    set(CONAN_PROFILE_OS "ios")
    list(GET CMAKE_OSX_ARCHITECTURES 0 CONAN_PROFILE_ARCH)
elseif(APPLE)
    set(CONAN_PROFILE_OS "macos")
    list(GET CMAKE_OSX_ARCHITECTURES 0 CONAN_PROFILE_ARCH)
elseif(WIN32)
    set(CONAN_PROFILE_OS "windows")
    if(DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_GENERATOR_PLATFORM}" CACHE STRING "" FORCE)
    elseif(DEFINED ENV{VSCMD_ARG_TGT_ARCH})
        set(CMAKE_SYSTEM_PROCESSOR "$ENV{VSCMD_ARG_TGT_ARCH}" CACHE STRING "" FORCE)
    endif()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(CONAN_PROFILE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|x64|amd64|AMD64")
        set(CONAN_PROFILE_ARCH "x86_64")
    else()
        set(CONAN_PROFILE_ARCH "x86")
    endif()
elseif(UNIX)
    set(CONAN_PROFILE_OS "linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(CONAN_PROFILE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|x64|amd64|AMD64")
        set(CONAN_PROFILE_ARCH "x86_64")
    else()
        set(CONAN_PROFILE_ARCH "x86")
    endif()
endif()

set(CONAN_PROFILE "${CONAN_PROFILE_OS}-${CONAN_PROFILE_ARCH}")

message(STATUS "CONAN_PROFILE_ARCH: ${CONAN_PROFILE_ARCH}")

# detect build host os/cpu architecture
if(CMAKE_HOST_APPLE)
    set(CONAN_BUILD_HOST "Macos")
    set(CONAN_BUILD_ARCH "x86_64")
elseif(CMAKE_HOST_WIN32)
    set(CONAN_BUILD_HOST "Windows")
    set(CONAN_BUILD_ARCH "x86_64")
elseif(CMAKE_HOST_UNIX)
    set(CONAN_BUILD_HOST "Linux")
    set(CONAN_BUILD_ARCH "x86_64")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(USE_CONAN)
    include(conan)

    conan_check()

    conan_cmake_install(
        CONANFILE conanfile.py
        PROFILE ${CONAN_PROFILE}
        GENERATORS cmake
        BUILD missing
        UPDATE
        SETTINGS "os_build=${CONAN_BUILD_HOST};arch_build=${CONAN_BUILD_ARCH};build_type=${CMAKE_BUILD_TYPE}"
    )

    set(CONAN_DISABLE_CHECK_COMPILER ON)
    conan_load_buildinfo()
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)

    if(CONAN_OUTPUT_DIRS)
        conan_output_dirs_setup()
    endif()
endif()
