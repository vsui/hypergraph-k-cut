add_library(sqlite3 sqlite3.c)
target_link_libraries(sqlite3 dl)
set(CMAKE_CXX_FLAGS " -pthread ")
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})