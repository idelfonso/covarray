#pragma once

#include "covarray/error.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

namespace covarray {

// A set of columns that must never simultaneously take a given set of
// values. `columns` and `values` have the same size; `values[i]` is the
// forbidden value for `columns[i]`. Example: if column 0 is "OS" (0 =
// Linux, 1 = Windows, 2 = macOS) and column 2 is "browser" (0 = Chrome,
// 1 = Firefox, 2 = Safari), `{.columns = {0, 2}, .values = {0, 2}}`
// forbids any row with OS = Linux and browser = Safari at the same time,
// since Safari doesn't run on Linux (every other OS/browser pairing is
// still allowed, including Safari on macOS and Chrome on Linux).
struct ForbiddenTuple {
    std::vector<std::uint8_t> columns;
    std::vector<std::uint8_t> values;
};

// Describes the covering array to construct. A `Spec` is only ever
// constructed validated: there is no way to hold one whose fields violate
// the invariants `create` checks.
//
// Deliberately absent: the number of rows N. N is the *output* of
// construction (the algorithm searches for a good, ideally minimal, N for
// the requested strength/levels), not an input describing what to cover —
// this is why the plan has a `bounds` command to estimate N from (t, k, v)
// rather than have the caller pick it. A finished `CoveringArray` carries
// its own N as its row count; a future fixed-N decision-problem search
// (e.g. simulated annealing asking "does an N-row CA exist?") would take N
// as a construction option, not add it here, since it's a parameter of
// *how* to search, not of *what* to cover.
class Spec {
  public:
    // `seed_rows`: rows a construction must include as-is (e.g. existing
    // regression tests or a previous array being extended) instead of
    // building from scratch; the rest of the array fills in whatever
    // t-tuples these rows don't already cover. Not yet used by anything —
    // construction doesn't exist yet — but already validated here (row
    // length and per-column value range) so it's ready when it does.
    [[nodiscard]] static auto create(
        unsigned strength, std::vector<std::uint8_t> levels,
        std::vector<ForbiddenTuple> forbidden_tuples = {},
        std::vector<std::vector<std::uint8_t>> seed_rows = {}
    ) -> std::expected<Spec, Error>;

    [[nodiscard]] auto strength() const noexcept -> unsigned {
        return strength_;
    }

    [[nodiscard]] auto levels() const noexcept -> std::span<const std::uint8_t> {
        return levels_;
    }

    [[nodiscard]] auto columns() const noexcept -> std::size_t {
        return levels_.size();
    }

    [[nodiscard]] auto forbidden_tuples() const noexcept -> std::span<const ForbiddenTuple> {
        return forbidden_tuples_;
    }

    [[nodiscard]] auto seed_rows() const noexcept -> std::span<const std::vector<std::uint8_t>> {
        return seed_rows_;
    }

  private:
    Spec(
        unsigned strength, std::vector<std::uint8_t> levels,
        std::vector<ForbiddenTuple> forbidden_tuples,
        std::vector<std::vector<std::uint8_t>> seed_rows
    );

    unsigned strength_;
    std::vector<std::uint8_t> levels_;
    std::vector<ForbiddenTuple> forbidden_tuples_;
    std::vector<std::vector<std::uint8_t>> seed_rows_;
};

} // namespace covarray
