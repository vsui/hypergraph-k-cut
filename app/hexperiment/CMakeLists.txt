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

target_link_libraries(hexperiment hypergraph sqlite3 yaml-cpp spdlog::spdlog tclap)

install(TARGETS hexperiment
        RUNTIME DESTINATION bin)
