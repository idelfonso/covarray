#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"
#include "covarray/verify.hpp"
#include "covarray/verify_report.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

namespace covarray {

namespace {

// Same hand-verified OA(4; 2, 3, 2) and 3-row truncation as verify_test.cpp
// -- every column pair sees all 4 value combinations in the full array,
// and exactly 3 missing tuples once truncated to 3 rows.
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

} // namespace

TEST(VerifyReport, FullyCoveringArrayReportsNoMissingTuples) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto array = make_fully_covered_array();
    const auto result = verify_index(array, *spec);

    const auto report = write_verification_report(array, *spec, result);

    EXPECT_NE(report.find("# N=4 k=3 t=2"), std::string::npos);
    EXPECT_NE(report.find("# levels: 2,2,2"), std::string::npos);
    EXPECT_NE(report.find("# total_tuples=12 missing=0 coverage=100.0000%"), std::string::npos);
    EXPECT_NE(report.find("# fully covering: no missing tuples"), std::string::npos);
    EXPECT_EQ(report.find("columns "), std::string::npos);
}

TEST(VerifyReport, TruncatedArrayListsEachMissingTupleGroupedByColumns) {
    auto spec = Spec::create(2, {2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    const auto truncated = make_truncated_array();
    const auto result = verify_index(truncated, *spec);
    ASSERT_EQ(result.missing.size(), 3);

    const auto report = write_verification_report(truncated, *spec, result);

    EXPECT_NE(report.find("# N=3 k=3 t=2"), std::string::npos);
    EXPECT_NE(report.find("# total_tuples=12 missing=3 coverage=75.0000%"), std::string::npos);

    // Combinations of {0,1,2} taken 2 at a time, in lexicographic order:
    // {0,1} rank 0, {0,2} rank 1, {1,2} rank 2 -- each has exactly one
    // missing tuple here, so each block has exactly one "values" line.
    const auto columns01 = report.find("columns 0,1 (combination_rank=0)");
    const auto values01 = report.find("values 1,1 (tuple_rank=3)");
    const auto columns02 = report.find("columns 0,2 (combination_rank=1)");
    // {0,2} and {1,2} both happen to be missing the same values/rank
    // ("1,0", tuple_rank 2), so the second search must start after its own
    // block's header -- otherwise both find the {0,2} block's line.
    const auto values02 = report.find("values 1,0 (tuple_rank=2)", columns02);
    const auto columns12 = report.find("columns 1,2 (combination_rank=2)");
    const auto values12 = report.find("values 1,0 (tuple_rank=2)", columns12);

    ASSERT_NE(columns01, std::string::npos);
    ASSERT_NE(values01, std::string::npos);
    ASSERT_NE(columns02, std::string::npos);
    ASSERT_NE(values02, std::string::npos);
    ASSERT_NE(columns12, std::string::npos);
    ASSERT_NE(values12, std::string::npos);

    // Each column block is immediately followed by its own values line,
    // and the blocks appear in combination-rank order.
    EXPECT_LT(columns01, values01);
    EXPECT_LT(values01, columns02);
    EXPECT_LT(columns02, values02);
    EXPECT_LT(values02, columns12);
    EXPECT_LT(columns12, values12);
}

} // namespace covarray
