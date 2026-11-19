#include "covarray/spec.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(Spec, ValidUniformSpecSucceeds) {
    auto spec = Spec::create(2, {3, 3, 3});
    ASSERT_TRUE(spec.has_value());
    EXPECT_EQ(spec->strength(), 2);
    EXPECT_EQ(spec->columns(), 3);
    EXPECT_EQ(spec->levels().size(), 3);
    EXPECT_TRUE(spec->forbidden_tuples().empty());
    EXPECT_TRUE(spec->seed_rows().empty());
}

TEST(Spec, ValidMixedLevelSpecSucceeds) {
    auto spec = Spec::create(2, {4, 3, 3, 2, 2, 2});
    ASSERT_TRUE(spec.has_value());
    EXPECT_EQ(spec->columns(), 6);
}

TEST(Spec, EmptyColumnsIsRejected) {
    auto spec = Spec::create(2, {});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::empty_columns);
}

TEST(Spec, LevelBelowTwoIsRejected) {
    auto spec = Spec::create(2, {3, 1, 3});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::invalid_level);
}

TEST(Spec, LevelOfZeroIsRejected) {
    auto spec = Spec::create(2, {3, 0});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::invalid_level);
}

TEST(Spec, MaxLevelOf255IsAccepted) {
    auto spec = Spec::create(2, {255, 255});
    ASSERT_TRUE(spec.has_value());
}

TEST(Spec, StrengthBelowTwoIsRejected) {
    auto spec = Spec::create(1, {3, 3});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::invalid_strength);
}

TEST(Spec, StrengthGreaterThanColumnsIsRejected) {
    auto spec = Spec::create(4, {3, 3, 3});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::invalid_strength);
}

TEST(Spec, StrengthEqualToColumnsIsAccepted) {
    auto spec = Spec::create(3, {3, 3, 3});
    ASSERT_TRUE(spec.has_value());
}

TEST(Spec, ForbiddenTupleWithMismatchedColumnsAndValuesIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {0, 1}, .values = {1}}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::forbidden_tuple_out_of_range);
}

TEST(Spec, EmptyForbiddenTupleIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {}, .values = {}}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::forbidden_tuple_out_of_range);
}

TEST(Spec, ForbiddenTupleColumnOutOfRangeIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {0, 3}, .values = {1, 1}}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::forbidden_tuple_out_of_range);
}

TEST(Spec, ForbiddenTupleValueOutOfRangeIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {0, 1}, .values = {1, 3}}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::forbidden_tuple_out_of_range);
}

TEST(Spec, ForbiddenTupleWithDuplicateColumnIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {0, 0}, .values = {1, 2}}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::forbidden_tuple_out_of_range);
}

TEST(Spec, ValidForbiddenTupleIsAccepted) {
    auto spec = Spec::create(2, {3, 3, 3}, {ForbiddenTuple{.columns = {0, 2}, .values = {1, 2}}});
    ASSERT_TRUE(spec.has_value());
    ASSERT_EQ(spec->forbidden_tuples().size(), 1);
}

TEST(Spec, SeedRowWithWrongColumnCountIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {}, {std::vector<std::uint8_t>{0, 1}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::seed_row_mismatch);
}

TEST(Spec, SeedRowWithValueOutOfRangeIsRejected) {
    auto spec = Spec::create(2, {3, 3, 3}, {}, {std::vector<std::uint8_t>{0, 1, 3}});
    ASSERT_FALSE(spec.has_value());
    EXPECT_EQ(spec.error().code, ErrorCode::seed_row_mismatch);
}

TEST(Spec, ValidSeedRowIsAccepted) {
    auto spec = Spec::create(
        2, {3, 3, 3}, {}, {std::vector<std::uint8_t>{0, 1, 2}, std::vector<std::uint8_t>{2, 1, 0}}
    );
    ASSERT_TRUE(spec.has_value());
    ASSERT_EQ(spec->seed_rows().size(), 2);
}

} // namespace covarray
