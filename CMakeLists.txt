cmake_minimum_required(VERSION 4.0)
project(V2P)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(source/libs/engine)

add_executable(V2P source/main.cpp)

target_link_libraries(V2P PRIVATE V2P_Engine)