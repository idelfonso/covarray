#include "covarray/verify.hpp"

#include "covarray/combinatorics.hpp"

#include <cassert>
#include <span>

namespace covarray {

namespace {

[[nodiscard]] auto product_of_levels(std::span<const std::uint8_t> levels) noexcept
    -> std::uint64_t {
    std::uint64_t total = 1;
    for (const auto level : levels) {
        total *= level;
    }
    return total;
}

[[nodiscard]] auto
levels_for(std::span<const std::uint8_t> levels, std::span<const std::size_t> columns)
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> subset(columns.size());
    for (std::size_t i = 0; i < columns.size(); ++i) {
        subset[i] = levels[columns[i]];
    }
    return subset;
}

[[nodiscard]] auto to_vector(std::span<const std::size_t> values) -> std::vector<std::size_t> {
    return {values.begin(), values.end()};
}

} // namespace

auto verify_reference(const CoveringArray& array, const Spec& spec) -> VerificationResult {
    assert(array.columns() == spec.columns());
    const auto levels = spec.levels();
    const auto rows = array.rows();

    VerificationResult result{.total_tuples = 0, .missing = {}};

    for_each_combination(
        array.columns(), spec.strength(), [&](std::span<const std::size_t> columns) -> void {
            const auto subset_levels = levels_for(levels, columns);
            const auto total_for_subset = product_of_levels(subset_levels);
            result.total_tuples += total_for_subset;

            for (std::uint64_t index = 0; index < total_for_subset; ++index) {
                auto values = unrank_tuple(index, subset_levels);

                bool covered = false;
                for (std::size_t r = 0; r < rows && !covered; ++r) {
                    bool row_matches = true;
                    for (std::size_t i = 0; i < columns.size(); ++i) {
                        if (array[r, columns[i]] != values[i]) {
                            row_matches = false;
                            break;
                        }
                    }
                    covered = row_matches;
                }

                if (!covered) {
                    result.missing.push_back(
                        MissingTuple{.columns = to_vector(columns), .values = std::move(values)}
                    );
                }
            }
        }
    );

    return result;
}

auto verify_index(const CoveringArray& array, const Spec& spec) -> VerificationResult {
    assert(array.columns() == spec.columns());
    const auto levels = spec.levels();
    const auto rows = array.rows();

    VerificationResult result{.total_tuples = 0, .missing = {}};

    for_each_combination(
        array.columns(), spec.strength(), [&](std::span<const std::size_t> columns) -> void {
            const auto subset_levels = levels_for(levels, columns);
            const auto total_for_subset = product_of_levels(subset_levels);
            result.total_tuples += total_for_subset;

            std::vector<bool> covered(total_for_subset, false);
            std::vector<std::uint8_t> row_values(columns.size());
            for (std::size_t r = 0; r < rows; ++r) {
                for (std::size_t i = 0; i < columns.size(); ++i) {
                    row_values[i] = array[r, columns[i]];
                }
                covered[rank_tuple(subset_levels, row_values)] = true;
            }

            for (std::uint64_t index = 0; index < total_for_subset; ++index) {
                if (!covered[index]) {
                    result.missing.push_back(
                        MissingTuple{
                            .columns = to_vector(columns),
                            .values = unrank_tuple(index, subset_levels),
                        }
                    );
                }
            }
        }
    );

    return result;
}

auto verify_fast(const CoveringArray& array, const Spec& spec) -> FastVerificationResult {
    assert(array.columns() == spec.columns());
    const auto levels = spec.levels();
    const auto rows = array.rows();
    const auto columns_count = array.columns();
    const auto strength = spec.strength();

    FastVerificationResult result{.first_missing = std::nullopt};
    if (strength > columns_count) {
        return result;
    }

    // A manual combination walk (rather than `for_each_combination`) so
    // finding a missing tuple can break out of the t-subset loop itself,
    // not just skip the work inside one iteration of it.
    std::vector<std::size_t> columns(strength);
    for (std::size_t i = 0; i < strength; ++i) {
        columns[i] = i;
    }

    do {
        const auto subset_levels = levels_for(levels, columns);
        const auto total_for_subset = product_of_levels(subset_levels);

        std::vector<bool> covered(total_for_subset, false);
        std::uint64_t covered_count = 0;
        std::vector<std::uint8_t> row_values(columns.size());
        for (std::size_t r = 0; r < rows && covered_count < total_for_subset; ++r) {
            for (std::size_t i = 0; i < columns.size(); ++i) {
                row_values[i] = array[r, columns[i]];
            }
            const auto index = rank_tuple(subset_levels, row_values);
            if (!covered[index]) {
                covered[index] = true;
                ++covered_count;
            }
        }

        if (covered_count < total_for_subset) {
            for (std::uint64_t index = 0; index < total_for_subset; ++index) {
                if (!covered[index]) {
                    result.first_missing = MissingTuple{
                        .columns = to_vector(columns),
                        .values = unrank_tuple(index, subset_levels),
                    };
                    break;
                }
            }
            break;
        }
    } while (next_combination(columns, columns_count));

    return result;
}

} // namespace covarray
