cmake_minimum_required(VERSION 3.10)

add_library(boost.ipc INTERFACE)
target_include_directories(boost.ipc INTERFACE include)
target_compile_definitions(boost.ipc INTERFACE BOOST_DATE_TIME_NO_LIB=1)
