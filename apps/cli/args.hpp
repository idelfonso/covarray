#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace covarray::cli {

enum class Subcommand : std::uint8_t { verify, info };

struct ParsedArgs {
    Subcommand command;
    std::string file;
    std::optional<unsigned> strength;               // only ever set for `verify`
    bool fast = false;                              // only ever set for `verify`
    std::optional<std::string> missing_tuples_path; // only ever set for `verify`
};

enum class ParseStatus : std::uint8_t { run, help, usage_error };

// What parsing argv produced: either arguments to run with, or text to
// print verbatim (`help` goes to stdout with exit 0, `usage_error` to
// stderr with ExitCode::usage_error) -- kept as plain data, like
// CommandResult, so parsing is testable without touching real argv or
// stdout/stderr.
struct ParseOutcome {
    ParseStatus status;
    std::string text;
    std::optional<ParsedArgs> args;
};

// Parses `args` (argv[1..], i.e. without the program name) into either a
// subcommand to run or text to print. Hand-rolls just enough of a
// traditional CLI contract for this program's two subcommands --
// `verify <file> [-t,--strength N]` and `info <file>` -- plus `-h,--help`
// at the top level or within a subcommand, so the program doesn't need an
// external argument-parsing library.
[[nodiscard]] auto parse_args(const std::vector<std::string>& args) -> ParseOutcome;

} // namespace covarray::cli
