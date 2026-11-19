# ADR-001: Monorepo

## Status

Accepted

## Context

The roadmap has covarray ship a C++23 core library, a Python package bound to
it, a CLI, and later gRPC/REST services and a WebAssembly playground. Each of
these depends tightly on the C++ core and tracks its release cadence.
Splitting them into separate repositories from the start would mean
cross-repo versioning, duplicated CI setup, and releases coordinated by hand
across repos.

## Decision

Keep the C++ core, Python bindings, CLI, services, and web playground in this
single `covarray` repository, organized by top-level directory (`include/`,
`src/`, `apps/cli/`, and later `python/`, `services/`, `web/`). Release tags
cover the whole repo; each component still carries its own version metadata
(the CMake project version, `pyproject.toml`, etc.) but is cut from the same
commit.

This is a separate decision from migrating the old standalone executables
that predate this project: that migration was deliberately skipped (nothing
in them implemented a covering-array construction method worth preserving),
not deferred into a `covarray-legacy` repo.

## Consequences

- One CI pipeline, one issue tracker, and atomic cross-component changes (a
  C++ API change and its Python binding can land in the same PR).
- The repo will accumulate multiple toolchains (CMake/vcpkg, uv/Python,
  Terraform, Docker) as phases land; CI will need path filters as it grows or
  build times will creep up for unrelated changes.
