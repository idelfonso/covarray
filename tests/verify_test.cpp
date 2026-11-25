#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"
#include "covarray/verify.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <ranges>
#include <vector>

namespace covarray {

namespace {

// OA(4; 2, 3, 2): every column pair sees all 4 value combinations across
// these 4 rows.
[[nodiscard]] auto make_fully_covered_array() -> CoveringArray {
    CoveringArray array(4, 3);
    const std::array<std::array<std::uint8_t, 3>, 4> rows{
        {{0, 0, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 0}}
    };
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            array[r, c] = rows[r][c];
        }
    }
    return array;
}

// The same 3-row truncation of `make_fully_covered_array()` used by
// `BothVerifiersListTheSameMissingTuplesForATruncatedArray`, factored out so
// `verify_fast`'s tests can reuse the same hand-verified instance.
[[nodiscard]] auto make_truncated_array() -> CoveringArray {
    const auto array = make_fully_covered_array();
    CoveringArray truncated(3, 3);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            truncated[r, c] = array[r, c];
        }
    }
    return truncated;
}

[[nodiscard]] auto contains_missing_tuple(
    const std::vector<MissingTuple>& missing, std::vector<std::size_t> columns,
    std::vector<std::uint8_t> values
) -> bool {
    return std::ranges::any_of(missing, [&](const MissingTuple& tuple) {
        return tuple.columns == columns && tuple.values == values;
    });
}

struct RandomInstance {
    CoveringArray array;
    Spec spec;
};

[[nodiscard]] auto make_random_instance(std::mt19937_64& rng) -> RandomInstance {
    std::uniform_int_distribution<std::size_t> columns_dist(2, 5);
    const auto columns = columns_dist(rng);
    std::uniform_int_distribution<unsigned> strength_dist(
        2, std::min<unsigned>(3, static_cast<unsigned>(columns))
    );
    const auto strength = strength_dist(rng);

    std::uniform_int_distribution<int> level_dist(2, 4);
    std::vector<std::uint8_t> levels(columns);
    for (auto& level : levels) {
        level = static_cast<std::uint8_t>(level_dist(rng));
    }
    auto spec = Spec::create(strength, levels);

    std::uniform_int_distribution<std::size_t> rows_dist(1, 12);
    const auto rows = rows_dist(rng);
    CoveringArray array(rows, columns);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < columns; ++c) {
            std::uniform_int_distribution<unsigned> value_dist(0, levels[c] - 1U);
            array[r, c] = static_cast<std::uint8_t>(value_dist(rng));
        }
    }
    return RandomInstance{.array = std::move(array), .spec = std::move(*spec)};
}

void expect_same_result(const VerificationResult& a, const VerificationResult& b) {
    EXPECT_EQ(a.total_tuples, b.total_tuples);
    ASSERT_EQ(a.missing.size(), b.missing.size());
    for (std::size_t i = 0; i < a.missing.size(); ++i) {
        EXPECT_EQ(a.missing[i].columns, b.missing[i].columns) << "i=" << i;
        EXPECT_EQ(a.missing[i].values, b.missing[i].values) << "i=" << i;
    }
}

// Column 2 is a wildcard in every row (value == its level, 2): per
// docs/design/ca-file-format.md, that column is never used to cover any
// tuple, so any subset including it is 0% covered while {0,1} alone is
// still fully covered by the first 4 rows.
[[nodiscard]] auto make_array_with_a_wildcard_column() -> CoveringArray {
    CoveringArray array(4, 3);
    const std::array<std::array<std::uint8_t, 3>, 4> rows{
        {{0, 0, 2}, {0, 1, 2}, {1, 0, 2}, {1, 1, 2}}
    };
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            array[r, c] = rows[r][c];
        }
    }
    return array;
}

} // namespace

TEST(Verify, IndexDoesNotCrashOnAWildcardCellAndExcludesItsSubsetsFromCoverage) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_array_with_a_wildcard_column();

    const auto result = verify_index(array, *spec);
    EXPECT_EQ(result.missing.size(), 8); // all 2*2*2 tuples over {0,2} and {1,2}
    EXPECT_FALSE(contains_missing_tuple(result.missing, {0, 1}, {0, 0}));
    EXPECT_TRUE(contains_missing_tuple(result.missing, {0, 2}, {0, 0}));
    EXPECT_TRUE(contains_missing_tuple(result.missing, {1, 2}, {0, 0}));
}

TEST(Verify, ReferenceAndIndexAgreeOnAWildcardCell) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_array_with_a_wildcard_column();

    const auto reference = verify_reference(array, *spec);
    const auto index = verify_index(array, *spec);
    expect_same_result(reference, index);
}

TEST(Verify, FastDoesNotCrashOnAWildcardCell) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_array_with_a_wildcard_column();

    const auto result = verify_fast(array, *spec);
    ASSERT_FALSE(result.is_covering());
    EXPECT_EQ(result.first_missing->columns, (std::vector<std::size_t>{0, 2}));
}

TEST(Verify, ReferenceFindsFullCoverage) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_fully_covered_array();

    const auto result = verify_reference(array, *spec);
    EXPECT_EQ(result.total_tuples, 12); // C(3,2) subsets * 2*2 tuples each
    EXPECT_TRUE(result.missing.empty());
    EXPECT_DOUBLE_EQ(result.coverage_percent(), 100.0);
}

TEST(Verify, IndexFindsFullCoverage) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_fully_covered_array();

    const auto result = verify_index(array, *spec);
    EXPECT_EQ(result.total_tuples, 12);
    EXPECT_TRUE(result.missing.empty());
    EXPECT_DOUBLE_EQ(result.coverage_percent(), 100.0);
}

TEST(Verify, BothVerifiersListTheSameMissingTuplesForATruncatedArray) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto truncated = make_truncated_array();

    const auto reference = verify_reference(truncated, *spec);
    const auto index = verify_index(truncated, *spec);
    expect_same_result(reference, index);

    EXPECT_EQ(reference.total_tuples, 12);
    EXPECT_EQ(reference.missing.size(), 3);
    EXPECT_DOUBLE_EQ(reference.coverage_percent(), 75.0);

    EXPECT_TRUE(contains_missing_tuple(reference.missing, {0, 1}, {1, 1}));
    EXPECT_TRUE(contains_missing_tuple(reference.missing, {0, 2}, {1, 0}));
    EXPECT_TRUE(contains_missing_tuple(reference.missing, {1, 2}, {1, 0}));
}

TEST(Verify, ReferenceAndIndexAgreeOnRandomInstances) {
    std::mt19937_64 rng(1234567);
    for (int iteration = 0; iteration < 1000; ++iteration) {
        auto instance = make_random_instance(rng);
        const auto reference = verify_reference(instance.array, instance.spec);
        const auto index = verify_index(instance.array, instance.spec);
        expect_same_result(reference, index);
    }
}

TEST(Verify, FastFindsFullCoverage) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_fully_covered_array();

    const auto result = verify_fast(array, *spec);
    EXPECT_TRUE(result.is_covering());
    EXPECT_FALSE(result.first_missing.has_value());
}

TEST(Verify, FastFindsAMissingTupleForATruncatedArray) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto truncated = make_truncated_array();

    const auto result = verify_fast(truncated, *spec);
    ASSERT_FALSE(result.is_covering());
    ASSERT_TRUE(result.first_missing.has_value());
    // Columns {0,1} is the lexicographically first pair and is itself
    // incomplete (rows only cover (0,0), (0,1), (1,0)), so it's what a
    // fail-fast walk must report.
    EXPECT_EQ(result.first_missing->columns, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(result.first_missing->values, (std::vector<std::uint8_t>{1, 1}));
}

TEST(Verify, FastAgreesWithIndexOnRandomInstances) {
    std::mt19937_64 rng(7654321);
    for (int iteration = 0; iteration < 1000; ++iteration) {
        auto instance = make_random_instance(rng);
        const auto index = verify_index(instance.array, instance.spec);
        const auto fast = verify_fast(instance.array, instance.spec);

        EXPECT_EQ(fast.is_covering(), index.missing.empty());
        if (!index.missing.empty()) {
            ASSERT_TRUE(fast.first_missing.has_value());
            // Both walk t-subsets in the same lexicographic order and pick
            // the smallest unmarked tuple index within one subset, so the
            // first failure a fail-fast walk reports is exactly the first
            // entry a full enumeration would have listed.
            EXPECT_EQ(fast.first_missing->columns, index.missing.front().columns);
            EXPECT_EQ(fast.first_missing->values, index.missing.front().values);
        }
    }
}

} // namespace covarray
