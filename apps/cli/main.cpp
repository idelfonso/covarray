#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <span>
#include <string>
#include <vector>

#include "args.hpp"
#include "commands.hpp"

using covarray::cli::CommandResult;
using covarray::cli::ExitCode;
using covarray::cli::ParseStatus;
using covarray::cli::Subcommand;

namespace {

[[nodiscard]] auto print_result(const CommandResult& result) -> int {
    auto* const stream = (result.code == ExitCode::success || result.code == ExitCode::not_covering)
                             ? stdout
                             : stderr;
    std::print(stream, "{}", result.message);
    return static_cast<int>(result.code);
}

// The actual program, split out of main() so main() itself never has a
// path that can throw (argument-parsing errors are already handled
// below; this just covers anything std::format/<filesystem>/etc. could
// still theoretically raise on top of that).
[[nodiscard]] auto run_main(int argc, char** argv) -> int {
    const std::span<char* const> all_args(argv, static_cast<std::size_t>(argc));
    const auto cli_args = all_args.subspan(1);
    const std::vector<std::string> args(cli_args.begin(), cli_args.end());
    const auto outcome = covarray::cli::parse_args(args);

    if (outcome.status == ParseStatus::help) {
        std::print(stdout, "{}", outcome.text);
        return 0;
    }
    if (outcome.status == ParseStatus::usage_error) {
        std::print(stderr, "{}", outcome.text);
        return static_cast<int>(ExitCode::usage_error);
    }

    const auto& parsed = *outcome.args;
    const CommandResult result = parsed.command == Subcommand::verify
                                     ? covarray::cli::run_verify(
                                           parsed.file,
                                           covarray::cli::VerifyOptions{
                                               .strength = parsed.strength,
                                               .fast = parsed.fast,
                                               .missing_tuples_path = parsed.missing_tuples_path,
                                           }
                                       )
                                     : covarray::cli::run_info(parsed.file);
    return print_result(result);
}

} // namespace

auto main(int argc, char** argv) -> int {
    try {
        return run_main(argc, argv);
    } catch (const std::exception& e) {
        std::print(stderr, "error: {}\n", e.what());
        return static_cast<int>(ExitCode::file_error);
    }
}
