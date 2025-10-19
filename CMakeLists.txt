cmake_minimum_required(VERSION 4.0)
project(V2P)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core)

# --- Build Target Switch ---
# This creates a new option for your CMake build.
# By default, it's OFF, so it will build the native SDL app.
option(BUILD_SERVER "Build the web server backend" OFF)

# --- Conditionally build the "head" ---
if(BUILD_SERVER)
    # User ran cmake with -DBUILD_SERVER=ON
    message(STATUS "Building backend server (app_server)")
    add_subdirectory(app_server)
else()
    # This is the default (BUILD_SERVER=OFF)
    message(STATUS "Building native application (app_sdl)")
    add_subdirectory(app_sdl)
endif()
