# ADR-002: Toolchain and dependency policy

## Status

Accepted

## Context

CI, the dev container, and the build system all have to commit to a specific
toolchain and a rule for how dependencies are added, before the codebase
grows large enough that changing course gets expensive.

## Decision

- **Language**: C++23, with `CMAKE_CXX_EXTENSIONS OFF`, enforced through
  `target_compile_features(... cxx_std_23)`.
- **Compilers**: GCC 16 and Clang 23, both built into the `covarray-dev`
  container image (`.devcontainer/Dockerfile`) and both required in CI
  (`build-test (gcc-16)`, `build-test (clang-23)`).
- **Build system**: CMake ≥ 3.28 with Ninja, driven through
  `CMakePresets.json` (`dev`, `release`, `asan-ubsan`); no hand-written
  Makefiles.
- **C++ dependencies**: vcpkg in manifest mode only (`vcpkg.json`). No system
  packages and no `FetchContent` for third-party libraries, so builds stay
  reproducible across the dev container and CI.
- **Testing**: GoogleTest for C++ unit tests, run through CTest.
- **Static analysis and style**: `.clang-format` and `.clang-tidy` are
  mandatory; the format check blocks CI, clang-tidy is advisory for now (see
  `ci.yml`), and both are enforced locally through
  `.pre-commit-config.yaml`.
- **Sanitizers**: ASan+UBSan is a first-class preset (`asan-ubsan`), not an
  optional add-on, and runs in CI on every PR.
- **Python toolchain** (from Phase 2 onward): `uv` for environments and tool
  management, consistent with the dev container.

## Consequences

- Anyone can reproduce CI locally with the same dev container and presets;
  no compiler drift between contributors.
- Contributors need GCC 16, Clang 23, and vcpkg available; the dev container
  is the supported way to get all three. Adding a new C++ dependency means a
  vcpkg manifest entry, which can lag behind a library's latest upstream
  release until vcpkg packages it.
