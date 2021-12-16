cmake_minimum_required(VERSION 3.21)
project(change_sql)

# Generate standalone Makefile
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
# Hide CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

add_subdirectory(3rd/fmt)
add_subdirectory(3rd/libuv)

add_executable(change_sql src/main.cpp)
target_link_libraries(change_sql fmt uv_a)
