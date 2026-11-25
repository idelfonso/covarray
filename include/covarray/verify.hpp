#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace covarray {

// One t-tuple of values over one t-subset of columns that no row of the
// array covers. `columns` is strictly increasing (matches
// `for_each_combination`'s order); `values[i]` is the missing value for
// `columns[i]`.
struct MissingTuple {
    std::vector<std::size_t> columns;
    std::vector<std::uint8_t> values;
};

// The result of checking an array against `spec.strength()`-way coverage:
// every t-subset of columns, crossed with every value tuple those
// columns' levels allow, is either covered by some row or listed in
// `missing`.
struct VerificationResult {
    std::uint64_t total_tuples;
    std::vector<MissingTuple> missing;

    [[nodiscard]] auto covered_tuples() const noexcept -> std::uint64_t {
        return total_tuples - missing.size();
    }

    [[nodiscard]] auto coverage_percent() const noexcept -> double {
        if (total_tuples == 0) {
            return 100.0;
        }
        return 100.0 * static_cast<double>(covered_tuples()) / static_cast<double>(total_tuples);
    }
};

// Deliberately naive: for every t-subset of `array`'s columns and every
// value tuple those columns' levels allow, linearly scans every row for a
// literal match. No indexing, no cleverness — slow, but obviously
// correct, so it's the differential-testing ground truth for
// `verify_index`. Precondition (asserted, not an `Error`: this is a
// caller bug, not untrusted input per ADR-003): `array.columns() ==
// spec.columns()`.
[[nodiscard]] auto verify_reference(const CoveringArray& array, const Spec& spec)
    -> VerificationResult;

// The "v^t bitset per t-set" approach: for every t-subset of `array`'s
// columns, one pass over the rows marks each row's projected value tuple
// (via `rank_tuple`) as covered in a bitset sized to that subset's
// levels' product, then every unmarked index becomes a `MissingTuple`
// (via `unrank_tuple`). Cost ~ C(k,t)*(N + v^t), the access pattern Thu
// Nov 26's parallel verifier splits across threads. Same precondition as
// `verify_reference`.
[[nodiscard]] auto verify_index(const CoveringArray& array, const Spec& spec) -> VerificationResult;

// The outcome of `verify_fast`: whether the array is `spec.strength()`-way
// covering, and, if not, one concrete counterexample.
struct FastVerificationResult {
    [[nodiscard]] auto is_covering() const noexcept -> bool {
        return !first_missing.has_value();
    }

    std::optional<MissingTuple> first_missing;
};

// A fail-fast covering check: two independent early exits, both exploiting
// that most covering arrays genuinely are covering, so the expected cost of
// a pass/fail question is far below enumerating every missing tuple.
//
// - Across t-subsets: stops enumerating columns entirely at the first
//   t-subset found to have any missing tuple — later subsets are never
//   examined, since the answer ("not covering") is already known.
// - Within one t-subset: stops scanning rows as soon as every one of that
//   subset's v^t tuples has been marked covered — later rows can't add
//   anything, since there's nothing left to cover.
//
// Reports only the first missing tuple found, not every one — this is a
// yes/no check, not an enumeration (`verify_reference`/`verify_index`
// remain the way to list every gap). Same precondition as
// `verify_reference`.
//
// Measured (full-factorial CA(3^10; 10, 3), a one-off `chrono` comparison,
// not committed benchmark infrastructure): on a covering instance,
// `verify_fast` beat `verify_index` by ~4x purely from the inner
// optimization (`verify_index` always takes one full N-row pass per
// subset; `verify_fast` only takes one when that subset never reaches full
// coverage). When the very first t-subset `for_each_combination` would
// visit was made non-covering, the outer optimization added another ~35x
// on top (~150x total vs `verify_index`) by never visiting the other 119
// subsets. When instead the *last* t-subset was the non-covering one, the
// outer optimization earned nothing (every other subset still had to be
// checked), so the result fell back to roughly the inner-only ~4x.
// `verify_reference` turned out faster than `verify_index` on this
// instance too, for an unrelated reason: its own per-tuple scan already
// exits on the first matching row, so it never pays for the unconditional
// full pass `verify_index` always takes.
[[nodiscard]] auto verify_fast(const CoveringArray& array, const Spec& spec)
    -> FastVerificationResult;

} // namespace covarray
