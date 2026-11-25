find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(covarray_cli_tests cli_test.cpp)
target_link_libraries(
    covarray_cli_tests
    PRIVATE
        covarray_cli_core
        covarray_warnings
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(covarray_cli_tests)
