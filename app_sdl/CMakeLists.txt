file(GLOB_RECURSE SOURCE_FILES source/*.cpp)
add_executable(V2P_SDL ${SOURCE_FILES})

find_package(SDL2 REQUIRED)

target_link_libraries(V2P_SDL
    PUBLIC
        V2P_Engine
        ${SDL2_LIBRARIES}
)
