add_executable(hcut main.cpp switch.hpp)
target_link_libraries(hcut hypergraph tclap)
install(TARGETS hcut
        RUNTIME DESTINATION bin)