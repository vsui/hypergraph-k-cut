add_subdirectory(sqlite)

set(CMAKE_CXX_FLAGS " -pthread ")

add_executable(hexperiment
        main.cpp
        common.hpp
        evaluator.cpp
        evaluator.hpp
        generators.cpp
        generators.hpp
        source.cpp
        source.hpp
        sqlutil.hpp
        store.cpp
        store.hpp)

target_link_libraries(hexperiment hypergraph sqlite3)

#add_executable(hgenerate generate.cpp generators.cpp)
#target_link_libraries(hgenerate hypergraph)
