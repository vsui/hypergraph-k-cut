set(CMAKE_CXX_FLAGS " -pthread ")

add_executable(hexperiment
        main.cpp
        common.hpp
        runner.cpp
        runner.hpp
        sqlutil.hpp
        store.cpp
        store.hpp
        experiment.cpp
        experiment.hpp
        sqlutil.cpp)

target_link_libraries(hexperiment
        hypergraph
        generators
        sqlite3
        yaml-cpp
        spdlog::spdlog
        tclap)

install(TARGETS hexperiment
        RUNTIME DESTINATION bin)
