cmake_minimum_required(VERSION 3.21)
project(change_sql)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
#string(APPEND CMAKE_LINKER_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
# Hide CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

add_subdirectory(3rd/spdlog)
add_subdirectory(3rd/libuv)
add_subdirectory(3rd/xxhash)
add_subdirectory(3rd/boost.ipc)
add_subdirectory(3rd/flat_hash_map)

add_executable(change_sql src/main.cpp src/mysql.cpp src/utils.cpp)
target_link_libraries(change_sql flat_hash_map spdlog xxhash boost.ipc rt uv_a crypto)

add_executable(lab lab/test.cpp)
target_link_libraries(lab spdlog flat_hash_map boost.ipc rt)
