cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project(HypergraphPartitioning)

add_executable(main main.cpp hypergraph.cpp order.cpp)
add_executable(gen gen.cpp hypergraph.cpp)

