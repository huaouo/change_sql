cmake_minimum_required(VERSION 3.8)

cmake_policy(SET CMP0048 NEW)                    ## set VERSION as documented by the project() command.
set(CMAKE_CXX_STANDARD 11)                       ## compile with C++11 support
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(phmap LANGUAGES CXX)

set(PHMAP_DIR ${PROJECT_SOURCE_DIR}/phmap)
set(PHMAP_HEADERS ${PHMAP_DIR}/phmap.h
        ${PHMAP_DIR}/phmap_base.h
        ${PHMAP_DIR}/phmap_bits.h
        ${PHMAP_DIR}/phmap_utils.h
        ${PHMAP_DIR}/phmap_config.h)
set(CMAKE_SUPPRESS_REGENERATION true)            ## suppress ZERO_CHECK project

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME} INTERFACE ${PHMAP_HEADERS})

target_include_directories(
        ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)
