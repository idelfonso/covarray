#include "covarray/verify_report.hpp"

#include "covarray/combinatorics.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <vector>

namespace covarray {

namespace {

[[nodiscard]] auto
levels_for(std::span<const std::uint8_t> levels, std::span<const std::size_t> columns)
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> subset(columns.size());
    for (std::size_t i = 0; i < columns.size(); ++i) {
        subset[i] = levels[columns[i]];
    }
    return subset;
}

template <class T> [[nodiscard]] auto join(std::span<const T> values) -> std::string {
    std::string out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        out += (i == 0) ? std::format("{}", values[i]) : std::format(",{}", values[i]);
    }
    return out;
}

} // namespace

auto write_verification_report(
    const CoveringArray& array, const Spec& spec, const VerificationResult& result
) -> std::string {
    const auto levels = spec.levels();
    std::string out;

    out += std::format(
        "# covarray verification report\n"
        "# N={} k={} t={}\n"
        "# levels: {}\n"
        "# total_tuples={} missing={} coverage={:.4f}%\n",
        array.rows(), array.columns(), spec.strength(), join(levels), result.total_tuples,
        result.missing.size(), result.coverage_percent()
    );

    if (result.missing.empty()) {
        out += "# fully covering: no missing tuples\n";
        return out;
    }

    out += "#\n"
           "# One block per t-combination of columns with missing tuples.\n"
           "# combination_rank/tuple_rank are the same lexicographic addressing\n"
           "# for_each_combination/rank_tuple use; either rank alone recovers the\n"
           "# explicit form via unrank_combination/unrank_tuple plus k/levels.\n";

    for (std::size_t i = 0; i < result.missing.size();) {
        const auto& columns = result.missing[i].columns;
        const auto combination_rank =
            rank_combination(std::span<const std::size_t>(columns), array.columns());
        out += std::format(
            "columns {} (combination_rank={})\n", join(std::span<const std::size_t>(columns)),
            combination_rank
        );

        const auto subset_levels = levels_for(levels, columns);
        std::size_t j = i;
        while (j < result.missing.size() && result.missing[j].columns == columns) {
            const auto& values = result.missing[j].values;
            const auto tuple_rank =
                rank_tuple(subset_levels, std::span<const std::uint8_t>(values));
            out += std::format(
                "  values {} (tuple_rank={})\n", join(std::span<const std::uint8_t>(values)),
                tuple_rank
            );
            ++j;
        }
        i = j;
    }

    return out;
}

} // namespace covarray
