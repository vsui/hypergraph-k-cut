
set(CMAKE_CXX_FLAGS " -pthread ")

set(CMAKE_CXX_STANDARD 17)

add_executable(hgenexperiment main.cpp)
target_link_libraries(hgenexperiment hypergraph generators)
install(TARGETS hgenexperiment
        RUNTIME DESTINATION bin)
