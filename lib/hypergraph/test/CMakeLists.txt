FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 482ac6ee63429af2aa9c44f4e6427873fb68fb1f)
FetchContent_MakeAvailable(googletest)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_executable(hypergraph_test
        suite.hpp
        testutil.hpp
        test.cpp
        metatest.cpp
        test_cut.cpp)
set_target_properties(hypergraph_test PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)
target_link_libraries(hypergraph_test gtest_main gmock_main hypergraph)

add_custom_target(test hypergraph_test)

