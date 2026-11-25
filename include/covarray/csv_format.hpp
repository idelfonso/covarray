#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/error.hpp"
#include "covarray/parsed_covering_array.hpp"
#include "covarray/spec.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace covarray {

// Parses `text` as a covering array CSV file: a header line
// `N,k,<level-spec>,t` (the same `v^c` level-spec grammar as the text
// matrix format, comma-separated instead of blank-separated), followed
// by `N` data rows of `k` comma-separated values (`0..v_i-1`, the
// wildcard `v_i`, or `-`). Mixed-level columns are reordered into
// decreasing-level order, matching `read_text_format`. No comments: CSV
// fields are plain integers, so there's nothing to escape and no reason
// to support them. Rejects the whole file — never a partial load — on a
// malformed header, an invalid cell symbol, or a row/column count that
// doesn't match the declared N/k.
[[nodiscard]] auto read_csv_format(std::string_view text)
    -> std::expected<ParsedCoveringArray, Error>;

// Writes `array` in the CSV format, using `spec`'s strength and
// per-column levels for the header line. Columns are written in
// whatever order `array`/`spec` already have them, matching
// `write_text_format`'s convention.
[[nodiscard]] auto write_csv_format(const CoveringArray& array, const Spec& spec) -> std::string;

} // namespace covarray
