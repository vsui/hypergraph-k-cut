add_executable(hgen gen.cpp)
target_link_libraries(hgen hypergraph)
install(TARGETS hgen
        RUNTIME DESTINATION bin)
