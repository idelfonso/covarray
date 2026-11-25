#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"
#include "covarray/verify.hpp"

#include <string>

namespace covarray {

// Formats every missing tuple in `result` as a human-readable report: a
// header giving the context needed to interpret anything below it (N, k,
// t, levels, and the overall total/missing/coverage numbers already on
// `result`), followed by one block per t-combination of `array`'s columns
// that has at least one missing tuple -- that combination's columns listed
// once, followed by every missing value tuple for it.
//
// Columns and values are always spelled out explicitly -- that's what
// makes a block readable without consulting anything else, which is the
// one requirement this report has to meet. Each line also carries a rank
// (`rank_combination` for a column block, `rank_tuple` for each value
// tuple -- the same lexicographic addressing `for_each_combination` and
// `verify_index` already use), not instead of the explicit form but
// alongside it: a rank is a compact, order-preserving key a later tool can
// sort, diff, or deduplicate on without re-parsing the explicit lists, and
// `unrank_combination`/`unrank_tuple` recovers the explicit form from it
// plus the header's k/levels -- so nothing is lost by keeping only the
// ranks in some other context. A fully rank-only report was considered and
// rejected for the report itself: a bare pair of integers isn't clear on
// its own.
//
// A fully covering array (`result.missing.empty()`) produces a report
// that says so explicitly in its header, not an empty or truncated file.
//
// This only formats the report; writing it to a file is a thin
// `std::ofstream` call left to the caller, matching how `write_csv_format`
// and friends only format their own on-disk representations too.
[[nodiscard]] auto write_verification_report(
    const CoveringArray& array, const Spec& spec, const VerificationResult& result
) -> std::string;

} // namespace covarray
