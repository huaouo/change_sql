cmake_minimum_required(VERSION 3.1...3.18)

# Fallback for using newer policies on CMake <3.12.
if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

# Joins arguments and places the results in ${result_var}.
function(join result_var)
    set(result "")
    foreach (arg ${ARGN})
        set(result "${result}${arg}")
    endforeach ()
    set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

include(CMakeParseArguments)

# Sets a cache variable with a docstring joined from multiple arguments:
#   set(<variable> <value>... CACHE <type> <docstring>...)
# This allows splitting a long docstring for readability.
function(set_verbose)
    # cmake_parse_arguments is broken in CMake 3.4 (cannot parse CACHE) so use
    # list instead.
    list(GET ARGN 0 var)
    list(REMOVE_AT ARGN 0)
    list(GET ARGN 0 val)
    list(REMOVE_AT ARGN 0)
    list(REMOVE_AT ARGN 0)
    list(GET ARGN 0 type)
    list(REMOVE_AT ARGN 0)
    join(doc ${ARGN})
    set(${var} ${val} CACHE ${type} ${doc})
endfunction()

project(FMT CXX)
include(GNUInstallDirs)
set_verbose(FMT_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING
        "Installation directory for include files, a relative path that "
        "will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute path.")

# Get version from core.h
file(READ include/fmt/core.h core_h)
if (NOT core_h MATCHES "FMT_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])")
    message(FATAL_ERROR "Cannot get FMT_VERSION from core.h.")
endif ()
# Use math to skip leading zeros if any.
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
join(FMT_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
        ${CPACK_PACKAGE_VERSION_PATCH})
message(STATUS "Version: ${FMT_VERSION}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

include(cxx14)
include(CheckCXXCompilerFlag)
include(JoinPaths)

list(FIND CMAKE_CXX_COMPILE_FEATURES "cxx_variadic_templates" index)
if (${index} GREATER -1)
    # Use cxx_variadic_templates instead of more appropriate cxx_std_11 for
    # compatibility with older CMake versions.
    set(FMT_REQUIRED_FEATURES cxx_variadic_templates)
endif ()
message(STATUS "Required features: ${FMT_REQUIRED_FEATURES}")

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(PEDANTIC_COMPILE_FLAGS -pedantic-errors -Wall -Wextra -pedantic
            -Wold-style-cast -Wundef
            -Wredundant-decls -Wwrite-strings -Wpointer-arith
            -Wcast-qual -Wformat=2 -Wmissing-include-dirs
            -Wcast-align
            -Wctor-dtor-privacy -Wdisabled-optimization
            -Winvalid-pch -Woverloaded-virtual
            -Wconversion -Wswitch-enum -Wundef
            -Wno-ctor-dtor-privacy -Wno-format-nonliteral)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS}
                -Wno-dangling-else -Wno-unused-local-typedefs)
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wdouble-promotion
                -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast
                -Wvector-operation-performance -Wsized-deallocation -Wshadow)
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2
                -Wnull-dereference -Wduplicated-cond)
    endif ()
    set(WERROR_FLAG -Werror)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -pedantic -Wconversion -Wundef
            -Wdeprecated -Wweak-vtables -Wshadow
            -Wno-gnu-zero-variadic-macro-arguments)
    check_cxx_compiler_flag(-Wzero-as-null-pointer-constant HAS_NULLPTR_WARNING)
    if (HAS_NULLPTR_WARNING)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS}
                -Wzero-as-null-pointer-constant)
    endif ()
    set(WERROR_FLAG -Werror)
endif ()

set(strtod_l_headers stdlib.h)

include(CheckSymbolExists)
check_symbol_exists(strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)

function(add_headers VAR)
    set(headers ${${VAR}})
    foreach (header ${ARGN})
        set(headers ${headers} include/fmt/${header})
    endforeach ()
    set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

# Define the fmt library, its includes and the needed defines.
add_headers(FMT_HEADERS args.h chrono.h color.h compile.h core.h format.h
        format-inl.h locale.h os.h ostream.h printf.h ranges.h
        xchar.h)
set(FMT_SOURCES src/format.cc src/os.cc)

add_library(fmt ${FMT_SOURCES} ${FMT_HEADERS})
add_library(fmt::fmt ALIAS fmt)

if (HAVE_STRTOD_L)
    target_compile_definitions(fmt PUBLIC FMT_LOCALE)
endif ()

target_compile_features(fmt INTERFACE ${FMT_REQUIRED_FEATURES})

target_include_directories(fmt PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${FMT_INC_DIR}>)

set_target_properties(fmt PROPERTIES
        VERSION ${FMT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}
        DEBUG_POSTFIX "${FMT_DEBUG_POSTFIX}")

# Set FMT_LIB_NAME for pkg-config fmt.pc. We cannot use the OUTPUT_NAME target
# property because it's not set by default.
set(FMT_LIB_NAME fmt)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FMT_LIB_NAME ${FMT_LIB_NAME}${FMT_DEBUG_POSTFIX})
endif ()

add_library(fmt-header-only INTERFACE)
add_library(fmt::fmt-header-only ALIAS fmt-header-only)

target_compile_definitions(fmt-header-only INTERFACE FMT_HEADER_ONLY=1)
target_compile_features(fmt-header-only INTERFACE ${FMT_REQUIRED_FEATURES})

target_include_directories(fmt-header-only INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${FMT_INC_DIR}>)

