#include "covarray/binary_format.hpp"
#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(BinaryFormat, UniformRoundTrip) {
    auto spec = Spec::create(2, {3, 3, 3});
    ASSERT_TRUE(spec.has_value());

    CoveringArray array(4, 3);
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            array[r, c] = static_cast<std::uint8_t>((r + c) % 3);
        }
    }

    const auto bytes = write_binary_format(array, *spec);
    auto parsed = read_binary_format(bytes);
    ASSERT_TRUE(parsed.has_value());

    EXPECT_EQ(parsed->spec.strength(), spec->strength());
    ASSERT_EQ(parsed->spec.levels().size(), spec->levels().size());
    for (std::size_t c = 0; c < spec->levels().size(); ++c) {
        EXPECT_EQ(parsed->spec.levels()[c], spec->levels()[c]);
    }

    ASSERT_EQ(parsed->array.rows(), array.rows());
    ASSERT_EQ(parsed->array.columns(), array.columns());
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            EXPECT_EQ((parsed->array[r, c]), (array[r, c])) << "r=" << r << " c=" << c;
        }
    }
}

TEST(BinaryFormat, WriteThenReadThenWriteIsIdempotent) {
    auto spec = Spec::create(2, {4, 3, 3, 2, 2, 2});
    ASSERT_TRUE(spec.has_value());

    CoveringArray array(2, 6);
    array[0, 0] = 3;
    array[0, 1] = 1;
    array[0, 2] = 0;
    array[0, 3] = 1;
    array[0, 4] = 0;
    array[0, 5] = 1;
    array[1, 0] = 4; // wildcard, numeric spelling
    array[1, 1] = 2;
    array[1, 2] = 1;
    array[1, 3] = 0;
    array[1, 4] = 1;
    array[1, 5] = 0;

    const auto bytes = write_binary_format(array, *spec);
    auto parsed = read_binary_format(bytes);
    ASSERT_TRUE(parsed.has_value());

    const auto bytes_again = write_binary_format(parsed->array, parsed->spec);
    EXPECT_EQ(bytes_again, bytes);
}

TEST(BinaryFormat, ColumnOrderIsPreservedNotReordered) {
    // Levels are NOT in decreasing order (2 before 4); unlike text/CSV,
    // the binary reader never reorders columns.
    auto spec = Spec::create(2, {2, 4, 3});
    ASSERT_TRUE(spec.has_value());
    CoveringArray array(1, 3);
    array[0, 0] = 1;
    array[0, 1] = 4;
    array[0, 2] = 2;

    const auto bytes = write_binary_format(array, *spec);
    auto parsed = read_binary_format(bytes);
    ASSERT_TRUE(parsed.has_value());

    const std::vector<std::uint8_t> expected_levels{2, 4, 3};
    ASSERT_EQ(parsed->spec.levels().size(), expected_levels.size());
    for (std::size_t c = 0; c < expected_levels.size(); ++c) {
        EXPECT_EQ(parsed->spec.levels()[c], expected_levels[c]) << "c=" << c;
    }
    EXPECT_EQ((parsed->array[0, 0]), 1);
    EXPECT_EQ((parsed->array[0, 1]), 4);
    EXPECT_EQ((parsed->array[0, 2]), 2);
}

TEST(BinaryFormat, ShortFileIsRejected) {
    auto parsed = read_binary_format("CVAR\x01");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(BinaryFormat, BadMagicIsRejected) {
    auto spec = Spec::create(2, {3, 3});
    ASSERT_TRUE(spec.has_value());
    CoveringArray array(1, 2);
    auto bytes = write_binary_format(array, *spec);
    bytes[0] = 'X';

    auto parsed = read_binary_format(bytes);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(BinaryFormat, UnsupportedVersionIsRejected) {
    auto spec = Spec::create(2, {3, 3});
    ASSERT_TRUE(spec.has_value());
    CoveringArray array(1, 2);
    auto bytes = write_binary_format(array, *spec);
    bytes[4] = static_cast<char>(99);

    auto parsed = read_binary_format(bytes);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(BinaryFormat, TruncatedBodyIsRejected) {
    auto spec = Spec::create(2, {3, 3});
    ASSERT_TRUE(spec.has_value());
    CoveringArray array(2, 2);
    auto bytes = write_binary_format(array, *spec);
    bytes.resize(bytes.size() - 1);

    auto parsed = read_binary_format(bytes);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(BinaryFormat, OutOfRangeSymbolIsRejected) {
    auto spec = Spec::create(2, {2, 2});
    ASSERT_TRUE(spec.has_value());
    CoveringArray array(1, 2);
    auto bytes = write_binary_format(array, *spec);
    bytes.back() = static_cast<char>(5); // level is 2, valid range is 0..2

    auto parsed = read_binary_format(bytes);
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
}

} // namespace covarray
