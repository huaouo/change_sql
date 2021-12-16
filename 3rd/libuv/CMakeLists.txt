cmake_minimum_required(VERSION 3.4)
project(libuv LANGUAGES C)

cmake_policy(SET CMP0057 NEW) # Enable IN_LIST operator
cmake_policy(SET CMP0064 NEW) # Support if (TEST) operator

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(CheckCCompilerFlag)
include(GNUInstallDirs)
include(CTest)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 90)

# Compiler check
string(CONCAT is-msvc $<OR:
        $<C_COMPILER_ID:MSVC>,
        $<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},MSVC>
        >)

check_c_compiler_flag(/W4 UV_LINT_W4)
check_c_compiler_flag(/wd4100 UV_LINT_NO_UNUSED_PARAMETER_MSVC)
check_c_compiler_flag(/wd4127 UV_LINT_NO_CONDITIONAL_CONSTANT_MSVC)
check_c_compiler_flag(/wd4201 UV_LINT_NO_NONSTANDARD_MSVC)
check_c_compiler_flag(/wd4206 UV_LINT_NO_NONSTANDARD_EMPTY_TU_MSVC)
check_c_compiler_flag(/wd4210 UV_LINT_NO_NONSTANDARD_FILE_SCOPE_MSVC)
check_c_compiler_flag(/wd4232 UV_LINT_NO_NONSTANDARD_NONSTATIC_DLIMPORT_MSVC)
check_c_compiler_flag(/wd4456 UV_LINT_NO_HIDES_LOCAL)
check_c_compiler_flag(/wd4457 UV_LINT_NO_HIDES_PARAM)
check_c_compiler_flag(/wd4459 UV_LINT_NO_HIDES_GLOBAL)
check_c_compiler_flag(/wd4706 UV_LINT_NO_CONDITIONAL_ASSIGNMENT_MSVC)
check_c_compiler_flag(/wd4996 UV_LINT_NO_UNSAFE_MSVC)

check_c_compiler_flag(-Wall UV_LINT_WALL) # DO NOT use this under MSVC

# TODO: Place these into its own function
check_c_compiler_flag(-Wno-unused-parameter UV_LINT_NO_UNUSED_PARAMETER)
check_c_compiler_flag(-Wstrict-prototypes UV_LINT_STRICT_PROTOTYPES)
check_c_compiler_flag(-Wextra UV_LINT_EXTRA)

check_c_compiler_flag(/utf-8 UV_LINT_UTF8_MSVC)

set(lint-no-unused-parameter $<$<BOOL:${UV_LINT_NO_UNUSED_PARAMETER}>:-Wno-unused-parameter>)
set(lint-strict-prototypes $<$<BOOL:${UV_LINT_STRICT_PROTOTYPES}>:-Wstrict-prototypes>)
set(lint-extra $<$<BOOL:${UV_LINT_EXTRA}>:-Wextra>)
set(lint-w4 $<$<BOOL:${UV_LINT_W4}>:/W4>)
set(lint-no-unused-parameter-msvc $<$<BOOL:${UV_LINT_NO_UNUSED_PARAMETER_MSVC}>:/wd4100>)
set(lint-no-conditional-constant-msvc $<$<BOOL:${UV_LINT_NO_CONDITIONAL_CONSTANT_MSVC}>:/wd4127>)
set(lint-no-nonstandard-msvc $<$<BOOL:${UV_LINT_NO_NONSTANDARD_MSVC}>:/wd4201>)
set(lint-no-nonstandard-empty-tu-msvc $<$<BOOL:${UV_LINT_NO_NONSTANDARD_EMPTY_TU_MSVC}>:/wd4206>)
set(lint-no-nonstandard-file-scope-msvc $<$<BOOL:${UV_LINT_NO_NONSTANDARD_FILE_SCOPE_MSVC}>:/wd4210>)
set(lint-no-nonstandard-nonstatic-dlimport-msvc $<$<BOOL:${UV_LINT_NO_NONSTANDARD_NONSTATIC_DLIMPORT_MSVC}>:/wd4232>)
set(lint-no-hides-local-msvc $<$<BOOL:${UV_LINT_NO_HIDES_LOCAL}>:/wd4456>)
set(lint-no-hides-param-msvc $<$<BOOL:${UV_LINT_NO_HIDES_PARAM}>:/wd4457>)
set(lint-no-hides-global-msvc $<$<BOOL:${UV_LINT_NO_HIDES_GLOBAL}>:/wd4459>)
set(lint-no-conditional-assignment-msvc $<$<BOOL:${UV_LINT_NO_CONDITIONAL_ASSIGNMENT_MSVC}>:/wd4706>)
set(lint-no-unsafe-msvc $<$<BOOL:${UV_LINT_NO_UNSAFE_MSVC}>:/wd4996>)
# Unfortunately, this one is complicated because MSVC and clang-cl support -Wall
# but using it is like calling -Weverything
string(CONCAT lint-default $<
        $<AND:$<BOOL:${UV_LINT_WALL}>,$<NOT:${is-msvc}>>:-Wall
        >)
set(lint-utf8-msvc $<$<BOOL:${UV_LINT_UTF8_MSVC}>:/utf-8>)

list(APPEND uv_cflags ${lint-strict-prototypes} ${lint-extra} ${lint-default} ${lint-w4})
list(APPEND uv_cflags ${lint-no-unused-parameter})
list(APPEND uv_cflags ${lint-no-unused-parameter-msvc})
list(APPEND uv_cflags ${lint-no-conditional-constant-msvc})
list(APPEND uv_cflags ${lint-no-nonstandard-msvc})
list(APPEND uv_cflags ${lint-no-nonstandard-empty-tu-msvc})
list(APPEND uv_cflags ${lint-no-nonstandard-file-scope-msvc})
list(APPEND uv_cflags ${lint-no-nonstandard-nonstatic-dlimport-msvc})
list(APPEND uv_cflags ${lint-no-hides-local-msvc})
list(APPEND uv_cflags ${lint-no-hides-param-msvc})
list(APPEND uv_cflags ${lint-no-hides-global-msvc})
list(APPEND uv_cflags ${lint-no-conditional-assignment-msvc})
list(APPEND uv_cflags ${lint-no-unsafe-msvc})
list(APPEND uv_cflags ${lint-utf8-msvc})

check_c_compiler_flag(-fno-strict-aliasing UV_F_STRICT_ALIASING)
list(APPEND uv_cflags $<$<BOOL:${UV_F_STRICT_ALIASING}>:-fno-strict-aliasing>)

set(uv_sources
        src/fs-poll.c
        src/idna.c
        src/inet.c
        src/random.c
        src/strscpy.c
        src/threadpool.c
        src/timer.c
        src/uv-common.c
        src/uv-data-getter-setters.c
        src/version.c
        src/unix/async.c
        src/unix/core.c
        src/unix/dl.c
        src/unix/fs.c
        src/unix/getaddrinfo.c
        src/unix/getnameinfo.c
        src/unix/loop-watcher.c
        src/unix/loop.c
        src/unix/pipe.c
        src/unix/poll.c
        src/unix/process.c
        src/unix/proctitle.c
        src/unix/random-devurandom.c
        src/unix/signal.c
        src/unix/stream.c
        src/unix/tcp.c
        src/unix/thread.c
        src/unix/tty.c
        src/unix/udp.c
        src/unix/linux-core.c
        src/unix/linux-inotify.c
        src/unix/linux-syscalls.c
        src/unix/procfs-exepath.c
        src/unix/random-getrandom.c
        src/unix/random-sysctl-linux.c
        src/unix/epoll.c)
list(APPEND uv_defines _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE _GNU_SOURCE _POSIX_C_SOURCE=200112)
list(APPEND uv_libraries pthread dl rt)

add_library(uv_a STATIC ${uv_sources})
target_compile_definitions(uv_a PRIVATE ${uv_defines})
target_compile_options(uv_a PRIVATE ${uv_cflags})
target_include_directories(uv_a
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
target_link_libraries(uv_a ${uv_libraries})
