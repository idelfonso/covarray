find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(covarray_tests combinatorics_test.cpp)
target_link_libraries(
    covarray_tests
    PRIVATE
        covarray::covarray
        covarray_warnings
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(covarray_tests)
