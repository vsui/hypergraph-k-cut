add_executable(hcut main.cpp builder.hpp)
target_link_libraries(hcut hypergraph tclap)
install(TARGETS hcut
        RUNTIME DESTINATION bin)