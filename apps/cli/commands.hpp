#pragma once

#include <optional>
#include <string>

#include "exit_code.hpp"

namespace covarray::cli {

// What a command produced: an exit code and the human-readable text to
// print (stdout on `success`/`not_covering`, stderr otherwise). Kept as
// plain returned data rather than printing directly, so these are
// testable without capturing real stdout/stderr or spawning the built
// binary -- `main.cpp` is the only place that actually writes it out.
struct CommandResult {
    ExitCode code;
    std::string message;
};

// `verify`'s own options, independent of how they were parsed from argv.
struct VerifyOptions {
    // Overrides the file's own embedded strength rather than requiring it
    // to match -- e.g. checking whether a t=3 array also happens to
    // satisfy t=2.
    std::optional<unsigned> strength;
    // Use `verify_fast`'s fail-fast check instead of the default
    // `verify_index` full enumeration: cheaper when the array is expected
    // to be covering, but reports only the first missing tuple rather
    // than every one. Mutually exclusive with `missing_tuples_path`,
    // which needs the full enumeration to have anything to write.
    bool fast = false;
    // When set, writes every missing tuple (`write_verification_report`)
    // to this path, in addition to the usual summary.
    std::optional<std::string> missing_tuples_path;
};

// Reads `path` and verifies it per `options`. Reports coverage % and,
// when the array isn't fully covering, a summary of the first few
// missing tuples (or, in fast mode, just the first one found).
[[nodiscard]] auto run_verify(const std::string& path, const VerifyOptions& options)
    -> CommandResult;

// Reads `path` and reports its N, k, t, and levels.
[[nodiscard]] auto run_info(const std::string& path) -> CommandResult;

} // namespace covarray::cli
