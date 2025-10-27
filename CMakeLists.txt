cmake_minimum_required(VERSION 3.20)
project(procgen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(procgen
        src/main.cpp
        include/noise.hpp
        include/camera.hpp
        shaders/noise_quad.vert
        shaders/noise_quad.frag
        shaders/terrain.vert
        shaders/terrain.frag
)

set_source_files_properties(
        shaders/noise_quad.vert shaders/noise_quad.frag
        shaders/terrain.vert shaders/terrain.frag
        PROPERTIES HEADER_FILE_ONLY TRUE
)

target_include_directories(procgen PRIVATE include)
target_link_libraries(procgen PRIVATE glfw glad::glad)
target_link_libraries(procgen PRIVATE glm::glm)

if (WIN32)
    target_compile_definitions(procgen PRIVATE NOMINMAX)
endif()
