cmake_minimum_required(VERSION 3.15)

project(Cadeau C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(WITH_SIMD "Use SIMD optimization" ON)
option(WITH_HALIDE "Use Halide optimization" ON)

option(WITH_XMF "Build Media Foundation library (XMF)" ON)
option(USE_CONAN "Use conan package manager" ${WITH_XMF})

option(CADEAU_INSTALL "Generate cmake installation script" ON)

include(ConanHelper)
include(CMakePackageConfigHelpers)

if(WIN32)
    include(WindowsRC)
endif()

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" VERSION_STRINGS)
list(GET VERSION_STRINGS 0 VERSION_STRING)

string(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+.[0-9]+" "\\1" CADEAU_VERSION_MAJOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+.[0-9]+" "\\1" CADEAU_VERSION_MINOR ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+).[0-9]+" "\\1" CADEAU_VERSION_PATCH ${VERSION_STRING})
string(REGEX REPLACE "[0-9]+.[0-9]+.[0-9]+.([0-9]+)" "\\1" CADEAU_VERSION_BUILD ${VERSION_STRING})
set(CADEAU_VERSION "${CADEAU_VERSION_MAJOR}.${CADEAU_VERSION_MINOR}.${CADEAU_VERSION_PATCH}.${CADEAU_VERSION_BUILD}")

set(CADEAU_VENDOR "Devolutions Inc.")
set(CADEAU_COPYRIGHT "Copyright 2022, ${CADEAU_VENDOR}")

if(NOT DEFINED MSVC_RUNTIME)
    set(MSVC_RUNTIME "static")
endif()

if(MSVC AND (MSVC_RUNTIME STREQUAL "static"))
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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

if(CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

set(CMAKE_EXPORT_DIR "lib/cmake/xpp")

# Halide

if(WITH_HALIDE)
    find_path(HALIDE_ROOT_DIR
        NAMES "include/Halide.h"
        HINTS
            ${HALIDE_ROOT_DIR}
            ENV HALIDE_ROOT_DIR)

    if(NOT HALIDE_ROOT_DIR)
        message(FATAL_ERROR "Halide could not be found! - set HALIDE_ROOT_DIR")
    endif()

    message(STATUS "HALIDE_ROOT_DIR: ${HALIDE_ROOT_DIR}")
    
    set(HALIDE_ROOT_DIR "${HALIDE_ROOT_DIR}" CACHE PATH "Halide root directory")
    set(HALIDE_INCLUDE_DIR "${HALIDE_ROOT_DIR}/include" CACHE PATH "Halide include directory")
    set(HALIDE_LIBRARY_DIR "${HALIDE_ROOT_DIR}/lib" CACHE PATH "Halide library directory")

    include_directories("${HALIDE_INCLUDE_DIR}")
endif()

# XPP: eXtreme Performance Primitives

add_definitions(-DXPP_EXPORTS)

include_directories(include)

add_subdirectory(libxpp)

# XMF: eXtreme Media Foundation

if(WITH_XMF)
    add_definitions(-DXMF_EXPORTS)

    include_directories(include)

    add_definitions(-DWITH_LIBVPX)
    add_definitions(-DWITH_LIBWEBM)

    add_subdirectory(libwebm)
    include_directories(libwebm)

    add_subdirectory(libxmf)
endif()

# installation

if(CADEAU_INSTALL)
    export(PACKAGE xpp)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cadeau-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cadeau-config.cmake"
        INSTALL_DESTINATION "${CMAKE_EXPORT_DIR}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/cadeau-config-version.cmake"
        VERSION ${CADEAU_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/xpp"
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/xmf"
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

    install(FILES
        ${PROJECT_BINARY_DIR}/cadeau-config.cmake
        ${PROJECT_BINARY_DIR}/cadeau-config-version.cmake
        DESTINATION "${CMAKE_EXPORT_DIR}")
endif()
