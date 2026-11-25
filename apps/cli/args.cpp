#include "args.hpp"

#include <charconv>
#include <cstddef>
#include <format>
#include <string_view>
#include <utility>

namespace covarray::cli {

namespace {

constexpr std::string_view top_level_usage =
    "covarray: construct, verify, and inspect covering arrays\n"
    "\n"
    "Usage: covarray <subcommand> [options]\n"
    "\n"
    "Subcommands:\n"
    "  verify  Verify a covering array file against its strength\n"
    "  info    Print a covering array file's N, k, t, and levels\n"
    "\n"
    "Run 'covarray <subcommand> --help' for subcommand options.\n";

constexpr std::string_view verify_help =
    "Usage: covarray verify <file> [-t,--strength N] [--fast] [-o,--missing-tuples FILE]\n"
    "\n"
    "Verify a covering array file against its strength.\n"
    "\n"
    "Arguments:\n"
    "  file                     Covering array file (.txt, .csv, .json, or .bin)\n"
    "\n"
    "Options:\n"
    "  -t,--strength N          Strength to verify at (default: the file's own declared\n"
    "                           strength)\n"
    "  --fast                   Fail-fast check: stop at the first missing tuple instead\n"
    "                           of enumerating every one (cannot be combined with\n"
    "                           -o,--missing-tuples)\n"
    "  -o,--missing-tuples FILE Write every missing tuple to FILE\n"
    "  -h,--help                Print this help and exit\n";

constexpr std::string_view info_help =
    "Usage: covarray info <file>\n"
    "\n"
    "Print a covering array file's N, k, t, and levels.\n"
    "\n"
    "Arguments:\n"
    "  file        Covering array file (.txt, .csv, .json, or .bin)\n"
    "\n"
    "Options:\n"
    "  -h,--help   Print this help and exit\n";

[[nodiscard]] auto usage_error(std::string message) -> ParseOutcome {
    return {.status = ParseStatus::usage_error, .text = std::move(message), .args = std::nullopt};
}

[[nodiscard]] auto help(std::string_view text) -> ParseOutcome {
    return {.status = ParseStatus::help, .text = std::string(text), .args = std::nullopt};
}

[[nodiscard]] auto is_help_flag(const std::string& token) -> bool {
    return token == "-h" || token == "--help";
}

// Parses a subcommand's own arguments: a required positional `file`,
// plus -- only for `verify`, since `info` has none of these --
// `-t,--strength N`, `--fast`, and `-o,--missing-tuples FILE`.
// `is_verify_subcommand` gates all three at once rather than needing a
// flag per option, since they're all verify-only.
[[nodiscard]] auto parse_subcommand_args(
    Subcommand command, std::string_view subcommand_name, std::string_view help_text,
    const std::vector<std::string>& rest, bool is_verify_subcommand
) -> ParseOutcome {
    for (const auto& token : rest) {
        if (is_help_flag(token)) {
            return help(help_text);
        }
    }

    std::optional<std::string> file;
    std::optional<unsigned> strength;
    std::optional<std::string> missing_tuples_path;
    bool fast = false;

    for (std::size_t i = 0; i < rest.size(); ++i) {
        const auto& token = rest[i];
        if (is_verify_subcommand && (token == "-t" || token == "--strength")) {
            if (i + 1 >= rest.size()) {
                return usage_error(std::format("error: {} requires a value\n", token));
            }
            const auto& value = rest[++i];
            unsigned parsed = 0;
            const auto* begin = value.data();
            const auto* end = value.data() + value.size();
            const auto parse_result = std::from_chars(begin, end, parsed);
            if (parse_result.ec != std::errc{} || parse_result.ptr != end) {
                return usage_error(
                    std::format(
                        "error: --strength must be a non-negative integer, got '{}'\n", value
                    )
                );
            }
            strength = parsed;
            continue;
        }
        if (is_verify_subcommand && token == "--fast") {
            fast = true;
            continue;
        }
        if (is_verify_subcommand && (token == "-o" || token == "--missing-tuples")) {
            if (i + 1 >= rest.size()) {
                return usage_error(std::format("error: {} requires a value\n", token));
            }
            missing_tuples_path = rest[++i];
            continue;
        }
        if (!token.empty() && token.front() == '-') {
            return usage_error(std::format("error: unrecognized option '{}'\n", token));
        }
        if (file.has_value()) {
            return usage_error(std::format("error: unexpected extra argument '{}'\n", token));
        }
        file = token;
    }

    if (!file.has_value()) {
        return usage_error(std::format("error: {} requires a file argument\n", subcommand_name));
    }

    return {
        .status = ParseStatus::run,
        .text = {},
        .args = ParsedArgs{
            .command = command,
            .file = *std::move(file),
            .strength = strength,
            .fast = fast,
            .missing_tuples_path = std::move(missing_tuples_path),
        },
    };
}

} // namespace

auto parse_args(const std::vector<std::string>& args) -> ParseOutcome {
    if (args.empty()) {
        return usage_error(std::string(top_level_usage));
    }
    if (is_help_flag(args.front())) {
        return help(top_level_usage);
    }

    const std::vector<std::string> rest(args.begin() + 1, args.end());
    if (args.front() == "verify") {
        return parse_subcommand_args(Subcommand::verify, "verify", verify_help, rest, true);
    }
    if (args.front() == "info") {
        return parse_subcommand_args(Subcommand::info, "info", info_help, rest, false);
    }

    return usage_error(
        std::format("error: unrecognized subcommand '{}'\n", args.front()) +
        std::string(top_level_usage)
    );
}

} // namespace covarray::cli
