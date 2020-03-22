cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(tclaplib
        GIT_REPOSITORY "https://github.com/vsui/tclap")
FetchContent_MakeAvailable(tclaplib)

project(HypergraphPartitioning)

add_subdirectory(hypergraph)
add_subdirectory(app)
add_subdirectory(experiment)
