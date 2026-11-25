# fuzz

libFuzzer harnesses that feed arbitrary bytes straight into covarray's
parsers, to catch crashes/UB on malformed input that no hand-written test
would think to try.

## CMake integration

Fuzzing is wired into the same preset-based toolchain as everything else,
across three pieces:

- **Root `CMakeLists.txt`**: `option(COVARRAY_BUILD_FUZZERS ...)`, off by
  default, gates `add_subdirectory(fuzz)` the same way
  `COVARRAY_BUILD_TESTS`/`COVARRAY_BUILD_CLI` gate `tests/`/`apps/cli/`.
- **`CMakePresets.json`**: the `fuzz` configure preset turns
  `COVARRAY_BUILD_FUZZERS` on and `COVARRAY_BUILD_TESTS`/
  `COVARRAY_BUILD_CLI` off, selects Clang (`--gcc-toolchain=/usr/local`,
  needed for Clang to see a `<mdspan>` that actually works here), and sets
  `-fsanitize=address,undefined` build-wide — so the `covarray` library
  itself, not just the fuzz target, is built with ASan+UBSan. There's a
  matching `fuzz` build preset (`cmake --build --preset fuzz` works too).
- **`fuzz/CMakeLists.txt`**: adds `-fsanitize=fuzzer` to
  `covarray_fuzz_text_format` alone, via `target_compile_options`/
  `target_link_options`, layering it on top of the preset's build-wide
  ASan+UBSan flags. It's kept out of the preset's global flags
  deliberately: libFuzzer supplies its own `main()`, which collides with
  every other executable's (the unit tests, the CLI, even CMake's own
  compiler-check program) if applied build-wide — that's a real error
  this setup hit and had to work around.

Nothing here touches the `dev`/`release`/`asan-ubsan`/`coverage` presets;
fuzzing lives entirely in its own preset and build directory
(`build/fuzz/`).

## Build

```bash
cmake --preset fuzz
cmake --build build/fuzz
```

This produces `build/fuzz/fuzz/covarray_fuzz_text_format`.

## Run

Point it at a corpus directory — it reads the seeds there and also writes
newly discovered interesting inputs back into that same directory, so use
a scratch copy rather than the tracked `corpus/text_format/` seeds
directly:

```bash
mkdir -p /tmp/fuzz_corpus
cp fuzz/corpus/text_format/*.txt /tmp/fuzz_corpus/
./build/fuzz/fuzz/covarray_fuzz_text_format -max_total_time=600 -artifact_prefix=/tmp/fuzz_artifacts/ /tmp/fuzz_corpus
```

- `-max_total_time=600` caps the run at 10 minutes; drop it to run until
  you `Ctrl-C`.
- `-artifact_prefix=...` is where it writes a `crash-<hash>` file if it
  finds one — point it somewhere outside the repo too.
- Omit the corpus directory entirely to fuzz from nothing (slower to get
  going, but works).

## If it finds a crash

It prints a stack trace and writes the failing input to
`<artifact_prefix>crash-<hash>`. Replay just that one input (a single
deterministic run, no fuzzing) with:

```bash
./build/fuzz/fuzz/covarray_fuzz_text_format /tmp/fuzz_artifacts/crash-<hash>
```
