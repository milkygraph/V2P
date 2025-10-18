cmake_minimum_required(VERSION 4.0)
project(V2P_Engine)

# --- Find FFmpeg ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libavutil
        libswscale
        libavfilter
        libavdevice
)

file(GLOB_RECURSE SOURCE_FILES *.cpp )

add_library(V2P_Engine ${SOURCE_FILES}
        source/stream/VideoStreamFactory.cpp
        source/stream/VideoStreamFactory.h
        source/writer/VideoWriter.cpp
        source/writer/VideoWriter.h)

target_link_libraries(V2P_Engine PRIVATE PkgConfig::LIBAV)
target_include_directories(V2P_Engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)