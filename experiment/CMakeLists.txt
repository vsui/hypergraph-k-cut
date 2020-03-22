add_subdirectory(sqlite)

set(CMAKE_CXX_FLAGS " -pthread ")

include(FetchContent)

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

FetchContent_Declare(yaml
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp"
        GIT_TAG "yaml-cpp-0.6.3")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing" FORCE)
FetchContent_MakeAvailable(yaml)
set(target_compile_options yaml-cpp PRIVATE -Wno-everything)

FetchContent_Declare(spdloglib
        GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
        GIT_TAG "v1.x")
FetchContent_MakeAvailable(spdloglib)
target_compile_options(spdlog PRIVATE -Wno-everything)

target_link_libraries(hexperiment hypergraph sqlite3 yaml-cpp spdlog::spdlog tclap)

#add_executable(hgenerate generate.cpp generators.cpp)
#target_link_libraries(hgenerate hypergraph)
