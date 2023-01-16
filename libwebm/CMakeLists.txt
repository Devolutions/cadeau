cmake_minimum_required(VERSION 3.2)

project(LIBWEBM CXX)

set(LIBWEBM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_LIMIT_MACROS" CACHE STRING "" FORCE)

include_directories("${LIBWEBM_SRC_DIR}")

set(libwebm_common_sources
    "${LIBWEBM_SRC_DIR}/common/file_util.cc"
    "${LIBWEBM_SRC_DIR}/common/file_util.h"
    "${LIBWEBM_SRC_DIR}/common/hdr_util.cc"
    "${LIBWEBM_SRC_DIR}/common/hdr_util.h"
    "${LIBWEBM_SRC_DIR}/common/webmids.h")

set(mkvmuxer_sources
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxer.cc"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxer.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxertypes.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxerutil.cc"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxerutil.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvwriter.cc"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvwriter.h")

set(mkvparser_sources
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvparser.cc"
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvparser.h"
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvreader.cc"
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvreader.h")

add_library(webm OBJECT
    ${libwebm_common_sources}
    ${mkvmuxer_sources}
    ${mkvparser_sources})
