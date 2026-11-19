# Contributing to covarray

covarray is in early development (see the [roadmap](README.md#roadmap)) and
isn't yet looking for outside contributions. The notes below are for the
maintainer and for anyone who wants to look under the hood in the meantime.
Questions and ideas are welcome in
[Discussions](https://github.com/idelfonso/covarray/discussions).

## Development environment

Open the repo in the provided [dev container](.devcontainer/) (VS Code
"Reopen in Container", or `devcontainer up`). It ships GCC 16, Clang 23,
CMake, Ninja, vcpkg, and `uv`, so there's no toolchain to install by hand.

## Building and testing

```sh
cmake --workflow --preset dev
```

Other presets: `release`, `asan-ubsan` (run this one before opening a PR —
CI requires it to pass).

## Before opening a PR

- Run `pre-commit run --all-files` (installed automatically by the dev
  container's `postCreateCommand`).
- Make sure `cmake --workflow --preset asan-ubsan` passes.
- Keep PRs scoped to one change — `main` only takes squash merges.

## Commit messages

Short, imperative subject line. Explain *why* in the body when it isn't
obvious from the diff.

## Architecture decisions

Non-obvious or hard-to-reverse technical decisions are recorded as ADRs
under `docs/adr/`. Propose new ones the same way: one file per decision,
with Status, Context, Decision, and Consequences sections.
