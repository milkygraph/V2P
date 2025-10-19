cmake_minimum_required(VERSION 4.0)
project(V2P_Engine)

# --- Find FFmpeg ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libavutil
        libswscale
        libavfilter
        libavdevice
        libswresample
)

file(GLOB_RECURSE SOURCE_FILES V2P/*.cpp)

add_library(V2P_Engine ${SOURCE_FILES})

target_link_libraries(V2P_Engine PUBLIC PkgConfig::FFMPEG)
target_include_directories(V2P_Engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
