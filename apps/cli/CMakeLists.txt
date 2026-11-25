# The actual argument parsing and verify/info logic, independent of
# real argv/stdout, so it's unit-testable without spawning the built
# binary as a subprocess.
add_library(covarray_cli_core STATIC args.cpp commands.cpp file_format.cpp)
target_include_directories(covarray_cli_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(covarray_cli_core PUBLIC covarray::covarray PRIVATE covarray_warnings)

add_executable(covarray_cli main.cpp)
set_target_properties(covarray_cli PROPERTIES OUTPUT_NAME covarray)
target_link_libraries(covarray_cli PRIVATE covarray_cli_core covarray_warnings)

if(COVARRAY_BUILD_TESTS)
    add_subdirectory(tests)
endif()
