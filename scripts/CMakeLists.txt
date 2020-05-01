add_executable(eps eps.cpp)

# TODO only need hana
find_package(Boost 1.53 REQUIRED)

# TODO get rid of all of these pthread's
set(CMAKE_CXX_FLAGS " -pthread ")

target_link_libraries(eps hypergraph generators Boost::boost)