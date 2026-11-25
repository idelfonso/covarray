# covarray

[![CI](https://github.com/idelfonso/covarray/actions/workflows/ci.yml/badge.svg)](https://github.com/idelfonso/covarray/actions/workflows/ci.yml)

**covarray** is a library for constructing, verifying and working with
[covering arrays](https://math.nist.gov/coveringarrays/coveringarray.html), the combinatorial
designs behind pairwise and t-way software testing. The core is written in modern
C++23, with a Python package on top, a command-line tool and, later, a
network API.

> **Status: early development.** The CLI's `verify`/`info` subcommands are usable
> (see Usage below); most of the library and roadmap items are still being built.

## Goals

- **Verify** covering arrays fast: count and list missing t-tuples, report coverage.
- **Construct** covering arrays with algebraic methods (orthogonal arrays over
  finite fields, CPHF-based constructions), greedy methods (IPOG) and
  metaheuristics (simulated annealing).
- **Generate test plans** from named parameters, values and constraints.
- **Use it from anywhere:** a C++ library, a Python package (`pip install covarray`),
  a CLI and a service API.
- **Engineer it properly:** tests, sanitizers, static analysis and measured
  performance from day one.

## Usage

Build the CLI with the `dev` preset:

```bash
cmake --preset dev
cmake --build build/dev --target covarray_cli
```

This produces `build/dev/apps/cli/covarray`.

`covarray verify` checks a covering array file against its own declared
strength, or an override:

```bash
covarray verify data/ipogf/t3_v2_k6_N14.txt
covarray verify data/ipogf/t3_v2_k6_N14.txt -t 2            # check against a different strength
covarray verify data/ipogf/t3_v2_k6_N14.txt --fast          # stop at the first missing tuple
covarray verify data/ipogf/t3_v2_k6_N14.txt -o missing.txt  # write every missing tuple to a file
```

`covarray info` prints a file's N, k, t and levels:

```bash
covarray info data/ipogf/t3_v2_k6_N14.txt
```

Supported file formats are `.txt`, `.csv`, `.json` and `.bin`, dispatched
by extension (see `docs/design/ca-file-format.md` for the text format).
Exit codes: `0` fully covering, `1` not covering, `2` usage error, `3`
file/parse error. Run `covarray <subcommand> --help` for the full option
list.

## Roadmap

| Phase | Target release | Focus |
| --- | --- | --- |
| P0 | Nov 2026 | Repository, build system, CI |
| P1 | v0.1.0 (Dec 2026) | Core types, file formats, parallel verification, CLI |
| P2 | v0.2.0 (Dec 2026) | Python package with wheels on PyPI |
| P3 | v0.3.0 (Dec 2026) | Algebraic constructions, IPOG, bounds |
| P4 | v0.4.0 (Dec 2026) | Public API and test-plan generation |
| P5 | v0.5.0 (Jan 2027) | LLM test-plan assistant and MCP server |
| P6 | v0.6.0 (Jan 2027) | Simulated annealing and performance work |
| P7 | v0.7.0 (Feb 2027) | gRPC and REST services, cloud deployment |
| P8 | v0.8.0 (Feb 2027) | ML-based method selection |
| P9 | v1.0.0 (Feb 2027) | Stable API, packaging, documentation |

Progress is tracked in the [project board](https://github.com/idelfonso/covarray/projects)
and the [milestones](https://github.com/idelfonso/covarray/milestones).

## Development

The project is developed in a VS Code dev container (see `.devcontainer/`), which
provides GCC 16, Clang 23, CMake, Ninja and the rest of the toolchain. Build and
test instructions will be added here as soon as the first code lands.

## Documentation

Architecture decisions and file-format specs live under `docs/`:

- [ADR-001: Monorepo](docs/adr/0001-monorepo.md) — why the C++ core, Python
  bindings, CLI, and future services all live in this one repository.
- [ADR-002: Toolchain and dependency policy](docs/adr/0002-toolchain-and-dependency-policy.md) —
  compilers, build system, and how a new C++ dependency gets added.
- [ADR-003: Errors as values](docs/adr/0003-errors-as-values.md) — why no
  covarray API throws.
- [Core types](docs/design/core-types.md) — `Error`, `Spec`, and
  `CoveringArray`'s storage layout and ownership rules.
- [Covering array text file format](docs/design/ca-file-format.md) — the
  `.txt`/`.csv` grammar, comments, and wildcard conventions.
- [Covering array binary file format](docs/design/ca-binary-format.md) —
  the versioned `.bin` layout.

## Contributing

The project is not ready for outside contributions yet. Questions and ideas are
welcome in [Discussions](https://github.com/idelfonso/covarray/discussions).

## License

Licensed under the [Apache License 2.0](LICENSE).
