cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project(HypergraphPartitioning)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(extern)

add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(scripts)
