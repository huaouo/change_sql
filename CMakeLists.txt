cmake_minimum_required(VERSION 3.21)
project(change_sql)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
#string(APPEND CMAKE_LINKER_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
# Hide CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

add_subdirectory(3rd/fmt)
add_subdirectory(3rd/libuv)

add_executable(change_sql src/main.cpp src/mysql.cpp)
target_link_libraries(change_sql fmt uv_a crypto)

add_executable(test src/test.cpp)
target_link_libraries(test fmt uv_a)
