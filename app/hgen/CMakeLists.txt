add_executable(hgen main.cpp)
target_link_libraries(hgen hypergraph tclap generators)
install(TARGETS hgen
        RUNTIME DESTINATION bin)
