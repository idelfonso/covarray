#include "commands.hpp"

#include "covarray/verify.hpp"
#include "covarray/verify_report.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <fstream>
#include <utility>
#include <vector>

#include "file_format.hpp"
#include "spec_formatter.hpp"

namespace covarray::cli {

namespace {

constexpr std::size_t max_missing_tuples_shown = 5;

[[nodiscard]] auto format_missing_tuple(const MissingTuple& tuple) -> std::string {
    std::string columns;
    std::string values;
    for (std::size_t i = 0; i < tuple.columns.size(); ++i) {
        columns +=
            (i == 0) ? std::format("{}", tuple.columns[i]) : std::format(",{}", tuple.columns[i]);
        values +=
            (i == 0) ? std::format("{}", tuple.values[i]) : std::format(",{}", tuple.values[i]);
    }
    return std::format("columns {} missing values {}", columns, values);
}

} // namespace

auto run_verify(const std::string& path, const VerifyOptions& options) -> CommandResult {
    if (options.fast && options.missing_tuples_path.has_value()) {
        return {
            .code = ExitCode::usage_error,
            .message = "error: --fast and --missing-tuples cannot be combined (fast "
                       "verification only ever finds the first missing tuple)\n",
        };
    }

    auto parsed = read_covering_array_file(path);
    if (!parsed.has_value()) {
        return {
            .code = ExitCode::file_error,
            .message = std::format("error: {}\n", parsed.error()),
        };
    }

    std::expected<Spec, Error> spec = parsed->spec;
    if (options.strength.has_value()) {
        spec = Spec::create(
            *options.strength,
            std::vector<std::uint8_t>(parsed->spec.levels().begin(), parsed->spec.levels().end()),
            std::vector<ForbiddenTuple>(
                parsed->spec.forbidden_tuples().begin(), parsed->spec.forbidden_tuples().end()
            ),
            std::vector<std::vector<std::uint8_t>>(
                parsed->spec.seed_rows().begin(), parsed->spec.seed_rows().end()
            )
        );
    }
    if (!spec.has_value()) {
        return {
            .code = ExitCode::usage_error,
            .message = std::format("error: {}\n", spec.error().message),
        };
    }

    if (options.fast) {
        const auto result = verify_fast(parsed->array, *spec);
        std::string message = std::format("{}\n", *spec);
        if (result.is_covering()) {
            message += "fully covering (fast check)\n";
            return {.code = ExitCode::success, .message = std::move(message)};
        }
        message += std::format(
            "not covering (fast check); first missing: {}\n",
            format_missing_tuple(*result.first_missing)
        );
        return {.code = ExitCode::not_covering, .message = std::move(message)};
    }

    const auto result = verify_index(parsed->array, *spec);
    std::string message = std::format(
        "{}\ncoverage: {:.4f}% ({}/{} tuples)\n", *spec, result.coverage_percent(),
        result.covered_tuples(), result.total_tuples
    );

    if (options.missing_tuples_path.has_value()) {
        std::ofstream report_file(*options.missing_tuples_path, std::ios::binary);
        if (!report_file) {
            return {
                .code = ExitCode::file_error,
                .message = std::format(
                    "error: cannot write missing-tuples report to '{}'\n",
                    *options.missing_tuples_path
                ),
            };
        }
        report_file << write_verification_report(parsed->array, *spec, result);
        message += std::format("missing tuples written to '{}'\n", *options.missing_tuples_path);
    }

    if (result.missing.empty()) {
        message += "fully covering\n";
        return {.code = ExitCode::success, .message = std::move(message)};
    }

    message += std::format(
        "missing {} tuple(s), showing up to {}:\n", result.missing.size(), max_missing_tuples_shown
    );
    const auto shown = std::min(result.missing.size(), max_missing_tuples_shown);
    for (std::size_t i = 0; i < shown; ++i) {
        message += "  " + format_missing_tuple(result.missing[i]) + "\n";
    }
    return {.code = ExitCode::not_covering, .message = std::move(message)};
}

auto run_info(const std::string& path) -> CommandResult {
    auto parsed = read_covering_array_file(path);
    if (!parsed.has_value()) {
        return {
            .code = ExitCode::file_error,
            .message = std::format("error: {}\n", parsed.error()),
        };
    }
    return {
        .code = ExitCode::success,
        .message = std::format("N={}\n{}\n", parsed->array.rows(), parsed->spec),
    };
}

} // namespace covarray::cli
