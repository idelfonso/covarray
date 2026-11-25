#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/error.hpp"
#include "covarray/parsed_covering_array.hpp"
#include "covarray/spec.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace covarray {

// Parses `bytes` as a covarray versioned binary file (see
// docs/design/ca-binary-format.md for the exact layout: magic bytes,
// version, little-endian N/k/t, per-column levels, then row-major cells).
// `bytes` is raw binary data, not text in any encoding. Unlike the
// text/CSV formats, there's no `-` wildcard spelling (a wildcard is
// always its numeric value) and no column reordering (this is a
// machine-interchange format with no hand-authoring convention to serve).
// Rejects the whole file — never a partial load — on a short read, bad
// magic bytes, an unsupported version, a length that doesn't match the
// header-declared N/k, or an invalid cell symbol.
[[nodiscard]] auto read_binary_format(std::string_view bytes)
    -> std::expected<ParsedCoveringArray, Error>;

// Writes `array` in the binary format above, using `spec`'s strength and
// per-column levels for the header, in whatever order `array`/`spec`
// already have them. The returned `std::string` is raw bytes, not text.
[[nodiscard]] auto write_binary_format(const CoveringArray& array, const Spec& spec) -> std::string;

} // namespace covarray
