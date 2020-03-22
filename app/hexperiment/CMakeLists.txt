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
set(YAML_CPP_INSTALL OFF CACHE BOOL "Build yaml-cpp shared library" FORCE)
FetchContent_MakeAvailable(yaml)
set(target_compile_options yaml-cpp PRIVATE -Wno-everything)

FetchContent_Declare(spdloglib
        GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
        GIT_TAG "v1.x")
FetchContent_MakeAvailable(spdloglib)
target_compile_options(spdlog PRIVATE -Wno-everything)

FetchContent_Declare(sqlite-amalgamation
        GIT_REPOSITORY "https://github.com/vsui/sqlite-amalgamation")
FetchContent_MakeAvailable(sqlite-amalgamation)
target_compile_options(sqlite3 PRIVATE -Wno-everything)

target_link_libraries(hexperiment hypergraph sqlite3 yaml-cpp spdlog::spdlog tclap)

install(TARGETS hexperiment
        RUNTIME DESTINATION bin)
