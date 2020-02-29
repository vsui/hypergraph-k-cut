add_subdirectory(sqlite)

set(CMAKE_CXX_FLAGS " -pthread ")

add_executable(hexperiment main.cpp source.cpp generators.cpp generators.hpp store.cpp evaluator.cpp)
target_link_libraries(hexperiment hypergraph sqlite3)

add_executable(hgenerate generate.cpp generators.cpp)
target_link_libraries(hgenerate hypergraph)
