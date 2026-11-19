#include "covarray/spec.hpp"

#include <algorithm>
#include <format>

namespace covarray {

Spec::Spec(
    unsigned strength, std::vector<std::uint8_t> levels,
    std::vector<ForbiddenTuple> forbidden_tuples, std::vector<std::vector<std::uint8_t>> seed_rows
)
    : strength_(strength), levels_(std::move(levels)),
      forbidden_tuples_(std::move(forbidden_tuples)), seed_rows_(std::move(seed_rows)) {}

namespace {

[[nodiscard]] auto validate_levels(std::span<const std::uint8_t> levels)
    -> std::expected<void, Error> {
    if (levels.empty()) {
        return std::unexpected(
            Error{
                .code = ErrorCode::empty_columns,
                .message = "Spec requires at least one column",
            }
        );
    }
    for (std::size_t column = 0; column < levels.size(); ++column) {
        if (levels[column] < 2) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::invalid_level,
                    .message = std::format(
                        "column {} has level {}, must be at least 2", column, levels[column]
                    ),
                }
            );
        }
    }
    return {};
}

[[nodiscard]] auto validate_strength(unsigned strength, std::size_t columns)
    -> std::expected<void, Error> {
    if (strength < 2) {
        return std::unexpected(
            Error{
                .code = ErrorCode::invalid_strength,
                .message = std::format("strength must be at least 2, got {}", strength),
            }
        );
    }
    if (strength > columns) {
        return std::unexpected(
            Error{
                .code = ErrorCode::invalid_strength,
                .message = std::format("strength {} exceeds column count {}", strength, columns),
            }
        );
    }
    return {};
}

[[nodiscard]] auto
validate_forbidden_tuple(const ForbiddenTuple& tuple, std::span<const std::uint8_t> levels)
    -> std::expected<void, Error> {
    if (tuple.columns.empty() || tuple.columns.size() != tuple.values.size()) {
        return std::unexpected(
            Error{
                .code = ErrorCode::forbidden_tuple_out_of_range,
                .message = "a forbidden tuple must have at least one column, "
                           "with one value per column",
            }
        );
    }
    for (std::size_t i = 0; i < tuple.columns.size(); ++i) {
        const auto column = tuple.columns[i];
        if (column >= levels.size()) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::forbidden_tuple_out_of_range,
                    .message = std::format(
                        "forbidden tuple references column {}, but there are "
                        "only {} columns",
                        column, levels.size()
                    ),
                }
            );
        }
        if (tuple.values[i] >= levels[column]) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::forbidden_tuple_out_of_range,
                    .message = std::format(
                        "forbidden tuple forbids value {} on column {}, but "
                        "that column's level is {}",
                        tuple.values[i], column, levels[column]
                    ),
                }
            );
        }
        const auto duplicate = std::ranges::find(
            tuple.columns.begin(), tuple.columns.begin() + static_cast<long>(i), column
        );
        if (duplicate != tuple.columns.begin() + static_cast<long>(i)) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::forbidden_tuple_out_of_range,
                    .message =
                        std::format("forbidden tuple references column {} more than once", column),
                }
            );
        }
    }
    return {};
}

[[nodiscard]] auto validate_seed_row(
    std::span<const std::uint8_t> row, std::size_t row_index, std::span<const std::uint8_t> levels
) -> std::expected<void, Error> {
    if (row.size() != levels.size()) {
        return std::unexpected(
            Error{
                .code = ErrorCode::seed_row_mismatch,
                .message = std::format(
                    "seed row {} has {} values, but there are {} columns", row_index, row.size(),
                    levels.size()
                ),
            }
        );
    }
    for (std::size_t column = 0; column < row.size(); ++column) {
        if (row[column] >= levels[column]) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::seed_row_mismatch,
                    .message = std::format(
                        "seed row {} has value {} on column {}, but that "
                        "column's level is {}",
                        row_index, row[column], column, levels[column]
                    ),
                }
            );
        }
    }
    return {};
}

} // namespace

auto Spec::create(
    unsigned strength, std::vector<std::uint8_t> levels,
    std::vector<ForbiddenTuple> forbidden_tuples, std::vector<std::vector<std::uint8_t>> seed_rows
) -> std::expected<Spec, Error> {
    if (auto result = validate_levels(levels); !result) {
        return std::unexpected(result.error());
    }
    if (auto result = validate_strength(strength, levels.size()); !result) {
        return std::unexpected(result.error());
    }
    for (const auto& tuple : forbidden_tuples) {
        if (auto result = validate_forbidden_tuple(tuple, levels); !result) {
            return std::unexpected(result.error());
        }
    }
    for (std::size_t row_index = 0; row_index < seed_rows.size(); ++row_index) {
        if (auto result = validate_seed_row(seed_rows[row_index], row_index, levels); !result) {
            return std::unexpected(result.error());
        }
    }
    return Spec(strength, std::move(levels), std::move(forbidden_tuples), std::move(seed_rows));
}

} // namespace covarray
