#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/error.hpp"
#include "covarray/spec.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace covarray {

// A covering array read from the plain-text matrix format, together with
// the strength and per-column levels its parameter line declared (see
// docs/design/ca-file-format.md for the grammar this is read against).
struct ParsedCoveringArray {
    CoveringArray array;
    Spec spec;
};

// Parses `text` as a covering array text file. Mixed-level columns are
// reordered into decreasing-level order (a stable sort, so columns
// sharing a level keep their relative order in `text`), matching the
// format's own output convention regardless of the order `text` used.
// Rejects the whole file — never a partial load — on a malformed
// parameter line, an invalid cell symbol, or a row/column count that
// doesn't match the declared N/k.
[[nodiscard]] auto read_text_format(std::string_view text)
    -> std::expected<ParsedCoveringArray, Error>;

// Writes `array` in the text matrix format, using `spec`'s strength and
// per-column levels for the parameter line. Columns are written in
// whatever order `array`/`spec` already have them: reordering happens
// once, at read time (see docs/design/ca-file-format.md), so writing
// what `read_text_format` just produced reproduces the same bytes.
[[nodiscard]] auto write_text_format(const CoveringArray& array, const Spec& spec) -> std::string;

} // namespace covarray
