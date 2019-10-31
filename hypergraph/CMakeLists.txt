find_package(Boost REQUIRED)

add_library(
        hypergraph
        STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bucket_list.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/certificate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hypergraph.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/order.cpp
)

target_include_directories(hypergraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hypergraph PUBLIC Boost::headers)

# Add warnings
target_compile_options(
        hypergraph
        PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wall -Werror -pedantic-errors -Wextra -Wconversion -Wsign-conversion>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /W4>
)

add_subdirectory(test)
