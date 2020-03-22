add_executable(hsparsify sparsify.cpp)
target_link_libraries(hsparsify hypergraph)
install(TARGETS hsparsify
        RUNTIME DESTINATION bin)
