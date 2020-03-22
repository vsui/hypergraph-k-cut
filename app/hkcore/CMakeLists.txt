add_executable(hkcore kcore.cpp)
target_link_libraries(hkcore hypergraph)
install(TARGETS hkcore
        RUNTIME DESTINATION bin)
