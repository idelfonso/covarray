#pragma once

#include <cstdint>

namespace covarray::cli {

// The CLI's own exit-code contract, documented in its --help output:
// 0 on success (including a fully-covering `verify`), 1 when `verify`
// ran cleanly but found the array isn't fully covering, 2 for a usage
// error (bad arguments, an invalid --strength), and 3 for a file or
// parse error (unreadable file, unrecognized extension, malformed
// content). "Not covering" gets its own code, distinct from a usage or
// parse error, so a caller can tell "the tool worked and the answer is
// no" apart from "the tool couldn't run."
enum class ExitCode : std::uint8_t {
    success = 0,
    not_covering = 1,
    usage_error = 2,
    file_error = 3,
};

} // namespace covarray::cli
