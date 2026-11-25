#pragma once

#include "covarray/parsed_covering_array.hpp"

#include <expected>
#include <string>

namespace covarray::cli {

// Reads the covering array file at `path`, dispatching to the text, CSV,
// JSON, or binary reader by its extension (.txt, .csv, .json, .bin
// respectively) -- a convention of this CLI, not something the formats
// themselves mandate (see docs/design/ca-file-format.md: "No extension is
// enforced"). An unreadable file, an unrecognized extension, and a
// malformed-content error from the underlying reader all surface here as
// a plain message: the CLI only ever prints it, never branches on an
// error code, so there's no reason to keep `covarray::Error`'s structured
// code around past this point.
[[nodiscard]] auto read_covering_array_file(const std::string& path)
    -> std::expected<ParsedCoveringArray, std::string>;

} // namespace covarray::cli
