add_library(
        generators
        STATIC
        include/generators/generators.hpp
        src/generators.cpp
)

target_include_directories(hypergraph PUBLIC include)
target_link_libraries(generators PUBLIC hypergraph)
