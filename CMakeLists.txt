cmake_minimum_required(VERSION 3.28)

project(
    covarray
    VERSION 0.1.0
    DESCRIPTION "C++23 and Python covering array library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This project doesn't use C++20 modules; disable Ninja's module dependency
# scanning so compile_commands.json stays a plain compile command instead of
# referencing a .modmap response file that only exists after a real build
# (clang-tidy configures but never builds, so that file is never there).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(COVARRAY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(covarray_warnings INTERFACE)
target_compile_options(
    covarray_warnings
    INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
            -Wshadow -Wold-style-cast -Wnull-dereference
            $<$<BOOL:${COVARRAY_WARNINGS_AS_ERRORS}>:-Werror>
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /permissive-
            $<$<BOOL:${COVARRAY_WARNINGS_AS_ERRORS}>:/WX>
        >
)

add_library(covarray src/covarray.cpp)
add_library(covarray::covarray ALIAS covarray)

target_include_directories(
    covarray
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(covarray PUBLIC cxx_std_23)
target_link_libraries(covarray PRIVATE covarray_warnings)

option(COVARRAY_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(COVARRAY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
