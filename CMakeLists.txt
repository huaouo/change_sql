cmake_minimum_required(VERSION 3.21)
project(change_sql)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
#string(APPEND CMAKE_LINKER_FLAGS_DEBUG " -fno-omit-frame-pointer -fsanitize=address,undefined")
# Hide CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

add_subdirectory(3rd/spdlog)
#add_subdirectory(3rd/libuv)
add_subdirectory(3rd/xxhash)
add_subdirectory(3rd/boost.ipc)
#add_subdirectory(3rd/mysqlclient)
add_subdirectory(3rd/flat_hash_map)

include_directories(/usr/include)
add_executable(run src/main.cpp src/utils.cpp src/buffered_reader.cpp src/query_builder.cpp)
target_link_libraries(run flat_hash_map spdlog xxhash boost.ipc
        rt crypto /usr/lib64/mysql/libmysqlclient.so)

add_executable(lab lab/test.cpp)
target_link_libraries(lab spdlog flat_hash_map boost.ipc rt)
