add_executable(hexperiment main.cpp source.cpp generators.cpp generators.hpp store.cpp evaluator.cpp)
target_link_libraries(hexperiment hypergraph)

add_executable(hgenerate generate.cpp generators.cpp)
target_link_libraries(hgenerate hypergraph)
