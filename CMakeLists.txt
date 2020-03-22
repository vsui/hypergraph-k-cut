cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(tclaplib
        GIT_REPOSITORY "https://github.com/vsui/tclap")
FetchContent_MakeAvailable(tclaplib)

project(HypergraphPartitioning)

add_subdirectory(hypergraph)

add_executable(hcut main.cpp)
target_link_libraries(hcut hypergraph tclap)

add_executable(hgen gen.cpp)
target_link_libraries(hgen hypergraph)

add_executable(hsparsify sparsify.cpp)
target_link_libraries(hsparsify hypergraph)

add_executable(hkcore kcore.cpp)
target_link_libraries(hkcore hypergraph)

install(TARGETS hgen hcut hkcore hsparsify
        RUNTIME DESTINATION bin)

add_subdirectory(experiment)

