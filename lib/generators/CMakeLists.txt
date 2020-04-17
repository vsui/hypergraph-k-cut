add_library(
        generators
        STATIC
        include/generators/generators.hpp
        src/generators.cpp
        ../../scripts/eps.cpp)

target_include_directories(hypergraph PUBLIC include)
target_link_libraries(generators PUBLIC hypergraph sqlite3)
