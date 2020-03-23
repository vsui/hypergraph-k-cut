include(FetchContent)

FetchContent_Declare(tclaplib
        GIT_REPOSITORY "https://github.com/vsui/tclap")
FetchContent_MakeAvailable(tclaplib)

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
