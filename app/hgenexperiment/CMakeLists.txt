add_executable(hgenexperiment main.cpp)
target_link_libraries(hgenexperiment hypergraph)
install(TARGETS hgenexperiment
        RUNTIME DESTINATION bin)
