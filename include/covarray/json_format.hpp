#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/error.hpp"
#include "covarray/parsed_covering_array.hpp"
#include "covarray/spec.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace covarray {

// Parses `text` as a covering array JSON document:
// `{"n": N, "k": k, "t": t, "levels": [v_0, ..., v_{k-1}],
//   "rows": [[...], ...]}` (`N` rows of `k` integers each). Unlike the
// text/CSV formats, there's no `-` wildcard spelling here (JSON values are
// typed, so there's no ambiguity to resolve, and no reason to borrow a
// text-only convention) — a wildcard is always written as its numeric
// value, same as the other formats' normalized form. Columns are kept in
// whatever order the document gives them: unlike the text/CSV formats,
// there's no hand-authoring convention motivating a canonical
// decreasing-level order for a machine-interchange format. Rejects the
// whole document — never a partial load — on invalid JSON, a missing or
// wrong-typed field, an invalid cell symbol, or a row/column count that
// doesn't match the declared N/k.
//
// A syntax error (malformed JSON itself) reports the line/column nlohmann
// already computes, folded into `Error::message`. A semantic error (a
// field with the wrong type, a bad cell) instead cites its position in
// the parsed *document structure* (a field name, a row/column index):
// JSON isn't line-oriented the way the text/CSV formats are, so there's
// no single, meaningful on-disk line/column to blame once parsing itself
// has already succeeded.
[[nodiscard]] auto read_json_format(std::string_view text)
    -> std::expected<ParsedCoveringArray, Error>;

// Writes `array` in the JSON format above, using `spec`'s strength and
// per-column levels, in whatever order `array`/`spec` already have them.
[[nodiscard]] auto write_json_format(const CoveringArray& array, const Spec& spec) -> std::string;

} // namespace covarray
