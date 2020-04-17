add_executable(eps eps.cpp)

# TODO only need hana
find_package(Boost 1.53 REQUIRED)

target_link_libraries(eps hypergraph generators Boost::boost)