
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
    if(NOT (CADEAU_CONAN_MAJOR STREQUAL "1" OR CADEAU_CONAN_MAJOR STREQUAL "2"))
        message(FATAL_ERROR "CADEAU_CONAN_MAJOR must be either '1' or '2'.")
    endif()

    if(CADEAU_CONAN_MAJOR STREQUAL "1")
        include(conan)

        find_program(CADEAU_CONAN1_COMMAND NAMES conan1 conan)
        if(NOT CADEAU_CONAN1_COMMAND)
            message(FATAL_ERROR "Could not find conan1 (or conan) in PATH.")
        endif()

        conan_cmake_install(
            CONAN_COMMAND ${CADEAU_CONAN1_COMMAND}
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
    else()
        find_program(CADEAU_CONAN2_COMMAND NAMES conan2)
        if(NOT CADEAU_CONAN2_COMMAND)
            message(FATAL_ERROR "Could not find conan2 in PATH.")
        endif()

        if(CONAN_PROFILE_OS STREQUAL "windows")
            set(CONAN2_PROFILE_OS "Windows")
        elseif(CONAN_PROFILE_OS STREQUAL "linux")
            set(CONAN2_PROFILE_OS "Linux")
        elseif(CONAN_PROFILE_OS STREQUAL "macos")
            set(CONAN2_PROFILE_OS "Macos")
        elseif(CONAN_PROFILE_OS STREQUAL "android")
            set(CONAN2_PROFILE_OS "Android")
        elseif(CONAN_PROFILE_OS STREQUAL "ios")
            set(CONAN2_PROFILE_OS "iOS")
        else()
            message(FATAL_ERROR "Unsupported Conan OS '${CONAN_PROFILE_OS}' for Conan 2.")
        endif()

        if(CONAN_PROFILE_ARCH MATCHES "arm64-v8a|aarch64|arm64|ARM64")
            set(CONAN2_PROFILE_ARCH "armv8")
        elseif(CONAN_PROFILE_ARCH MATCHES "armeabi-v7a|armv7|arm")
            set(CONAN2_PROFILE_ARCH "armv7")
        elseif(CONAN_PROFILE_ARCH MATCHES "x86_64|x64|amd64|AMD64")
            set(CONAN2_PROFILE_ARCH "x86_64")
        else()
            set(CONAN2_PROFILE_ARCH "x86")
        endif()

        set(CONAN2_INSTALL_ARGS
            install
            "${CMAKE_SOURCE_DIR}/conanfile_conan2.py"
            "-of=${CMAKE_CURRENT_BINARY_DIR}"
            "-g" "CMakeDeps"
            "-g" "CMakeToolchain"
            "--build=missing"
            "--update"
            "-s:h" "os=${CONAN2_PROFILE_OS}"
            "-s:h" "arch=${CONAN2_PROFILE_ARCH}"
            "-s:h" "build_type=${CMAKE_BUILD_TYPE}"
            "-s:b" "os=${CONAN_BUILD_HOST}"
            "-s:b" "arch=${CONAN_BUILD_ARCH}"
        )

        message(STATUS "Conan 2 executing: ${CADEAU_CONAN2_COMMAND} ${CONAN2_INSTALL_ARGS}")
        execute_process(
            COMMAND ${CADEAU_CONAN2_COMMAND} ${CONAN2_INSTALL_ARGS}
            RESULT_VARIABLE CONAN2_RESULT
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        if(NOT "${CONAN2_RESULT}" STREQUAL "0")
            message(FATAL_ERROR "Conan 2 install failed='${CONAN2_RESULT}'")
        endif()

        set(CONAN2_LEGACY_FILE "${CMAKE_CURRENT_BINARY_DIR}/generators/conandeps_legacy.cmake")
        if(NOT EXISTS "${CONAN2_LEGACY_FILE}")
            message(FATAL_ERROR "Conan 2 did not generate ${CONAN2_LEGACY_FILE}")
        endif()

        include("${CONAN2_LEGACY_FILE}")
        set(CONAN_TARGETS ${CONANDEPS_LEGACY})
    endif()
endif()
