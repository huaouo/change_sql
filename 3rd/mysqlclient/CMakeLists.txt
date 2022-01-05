cmake_minimum_required(VERSION 3.10)
project(mysqlclient LANGUAGES C)

add_library(mysqlclient STATIC IMPORTED GLOBAL)
set_property(TARGET mysqlclient PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/lib/libmysqlclient.a")
target_include_directories(mysqlclient INTERFACE include)
