cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project(HypergraphPartitioning)

add_subdirectory(hypergraph)

add_executable(hcut main.cpp)
target_include_directories(hcut PRIVATE ${CMAKE_SOURCE_DIR}/ext)
target_link_libraries(hcut hypergraph)

add_executable(hgen gen.cpp)
target_link_libraries(hgen hypergraph)

add_executable(hsparsify sparsify.cpp)
target_link_libraries(hsparsify hypergraph)
