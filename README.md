# covarray

[![CI](https://github.com/idelfonso/covarray/actions/workflows/ci.yml/badge.svg)](https://github.com/idelfonso/covarray/actions/workflows/ci.yml)

**covarray** is a library for constructing, verifying and working with
[covering arrays](https://math.nist.gov/coveringarrays/coveringarray.html), the combinatorial
designs behind pairwise and t-way software testing. The core is written in modern
C++ (C++23), with a Python package on top, a command-line tool and, later, a
network API.

> **Status: early development.** The repository is being set up; there is nothing
> to install or use yet. The roadmap below shows what is coming and when.

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
## Roadmap

| Phase | Target release | Focus |
| --- | --- | --- |
| P0 | Jan 2027 | Repository, build system, CI |
| P1 | v0.1.0 (Feb 2027) | Core types, file formats, parallel verification, CLI |
| P2 | v0.2.0 (Feb 2027) | Python package with wheels on PyPI |
| P3 | v0.3.0 (Mar 2027) | Algebraic constructions, IPOG, bounds |
| P4 | v0.4.0 (Apr 2027) | Public API and test-plan generation |
| P5 | v0.5.0 (Apr 2027) | LLM test-plan assistant and MCP server |
| P6 | v0.6.0 (May 2027) | Simulated annealing and performance work |
| P7 | v0.7.0 (Jun 2027) | gRPC and REST services, cloud deployment |
| P8 | v0.8.0 (Jul 2027) | ML-based method selection |
| P9 | v1.0.0 (Jul 2027) | Stable API, packaging, documentation |

Progress is tracked in the [project board](https://github.com/idelfonso/covarray/projects)
and the [milestones](https://github.com/idelfonso/covarray/milestones).

## Development

The project is developed in a VS Code dev container (see `.devcontainer/`), which
provides GCC 16, Clang 23, CMake, Ninja and the rest of the toolchain. Build and
test instructions will be added here as soon as the first code lands.

## Contributing

The project is not ready for outside contributions yet. Questions and ideas are
welcome in [Discussions](https://github.com/idelfonso/covarray/discussions).

## License

Licensed under the [Apache License 2.0](LICENSE).
