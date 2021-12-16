cmake_minimum_required(VERSION 3.21)
project(change_sql)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(3rd/fmt)

add_executable(change_sql main.cpp)
target_link_libraries(change_sql fmt)
