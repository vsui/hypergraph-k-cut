add_executable(hexperiment main.cpp source.cpp generators.cpp generators.hpp store.cpp evaluator.cpp)
target_link_libraries(hexperiment hypergraph)