cmake_minimum_required(VERSION 3.10)

add_library(xxhash INTERFACE)
target_include_directories(xxhash INTERFACE include)
target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL=1)
