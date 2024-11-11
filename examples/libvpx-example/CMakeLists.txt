# CMakeLists.txt for libvpx-example
cmake_minimum_required(VERSION 3.14)
project(libvpx_example)

option(WITH_XMF "Build Media Foundation library (XMF)" ON)
option(USE_CONAN "Use conan package manager" ${WITH_XMF})

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)

# Include Conan before setting include and link directories
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Specify the source files
add_executable(libvpx_example src/main.cpp)

# Include directories
target_include_directories(libvpx_example PRIVATE
    ${CMAKE_SOURCE_DIR}/../../include # General include directory
    ${CMAKE_SOURCE_DIR}/../../libwebm # libwebm headers
    ${CMAKE_SOURCE_DIR}/../../libxpp # libxpp headers
    ${CMAKE_SOURCE_DIR}/../../libxmf # libxmf headers
    ${CONAN_INCLUDE_DIRS}
)

# Link directories for the required libraries
target_link_directories(libvpx_example PRIVATE
    ${CMAKE_SOURCE_DIR}/../../build/libwebm/webm.dir/Debug # Path to webm.lib
    ${CMAKE_SOURCE_DIR}/../../build/libxpp/Debug           # Path to xpp.lib
    ${CMAKE_SOURCE_DIR}/../../build/libxmf/Debug           # Path to xmf.lib
    ${CONAN_LIB_DIRS}
)

# Link libraries
target_link_libraries(libvpx_example PRIVATE
    webm # Link webm.lib
    xpp  # Link xpp.lib
    xmf  # Link xmf.lib (added this line)
    ${CONAN_LIBS} # Link libraries from Conan (libvpx)
)


add_custom_command(TARGET libvpx_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/../../build/libxmf/Debug/xmf.dll
    $<TARGET_FILE_DIR:libvpx_example>
)

# Set runtime output directory to ensure the executable and DLLs are together
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../../build/libvpx-example/Debug)

# Set up runtime DLL path for xmf when debugging in Visual Studio
if(WIN32)
    set_target_properties(libvpx_example PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${CMAKE_SOURCE_DIR}/../../build/libxmf/Debug"
    )
endif()
