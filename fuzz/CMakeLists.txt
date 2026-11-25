add_executable(covarray_fuzz_text_format text_format_fuzzer.cpp)
# -fsanitize=fuzzer is scoped to this target alone, not the whole build
# (see the `fuzz` preset): libFuzzer supplies its own main(), which would
# collide with every other executable's if applied globally.
target_compile_options(covarray_fuzz_text_format PRIVATE -fsanitize=fuzzer)
target_link_options(covarray_fuzz_text_format PRIVATE -fsanitize=fuzzer)
target_link_libraries(covarray_fuzz_text_format PRIVATE covarray::covarray)
