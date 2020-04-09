set(CMAKE_CXX_FLAGS " -pthread ")

add_executable(hexperiment
        main.cpp
        common.hpp
        runner.cpp
        runner.hpp
        generators.cpp
        generators.hpp
        sqlutil.hpp
        store.cpp
        store.hpp
        experiment.cpp
        experiment.hpp)

target_link_libraries(hexperiment hypergraph sqlite3 yaml-cpp spdlog::spdlog tclap)

install(TARGETS hexperiment
        RUNTIME DESTINATION bin)
