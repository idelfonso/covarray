#include "covarray/covering_array.hpp"
#include "covarray/csv_format.hpp"
#include "covarray/spec.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(CsvFormat, UniformRoundTrip) {
    auto spec = Spec::create(2, {3, 3, 3});
    ASSERT_TRUE(spec.has_value());

    CoveringArray array(4, 3);
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            array[r, c] = static_cast<std::uint8_t>((r + c) % 3);
        }
    }

    const auto text = write_csv_format(array, *spec);
    auto parsed = read_csv_format(text);
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

TEST(CsvFormat, WriteThenReadThenWriteIsIdempotent) {
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

    const auto text = write_csv_format(array, *spec);
    auto parsed = read_csv_format(text);
    ASSERT_TRUE(parsed.has_value());

    const auto text_again = write_csv_format(parsed->array, parsed->spec);
    EXPECT_EQ(text_again, text);
}

TEST(CsvFormat, ReadReordersMixedLevelColumnsToDecreasingOrder) {
    const auto parsed = read_csv_format("1,6,2^3,3^2,4^1,2\n1,0,2,2,3,4\n");
    ASSERT_TRUE(parsed.has_value());

    const std::vector<std::uint8_t> expected_levels{4, 3, 3, 2, 2, 2};
    ASSERT_EQ(parsed->spec.levels().size(), expected_levels.size());
    for (std::size_t c = 0; c < expected_levels.size(); ++c) {
        EXPECT_EQ(parsed->spec.levels()[c], expected_levels[c]) << "c=" << c;
    }

    const std::vector<std::uint8_t> expected_row{4, 2, 3, 1, 0, 2};
    for (std::size_t c = 0; c < expected_row.size(); ++c) {
        EXPECT_EQ((parsed->array[0, c]), expected_row[c]) << "c=" << c;
    }
}

TEST(CsvFormat, HyphenIsNormalizedToNumericWildcard) {
    auto parsed = read_csv_format("1,2,3^2,2\n-,1\n");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ((parsed->array[0, 0]), 3);
    EXPECT_EQ((parsed->array[0, 1]), 1);
}

TEST(CsvFormat, WhitespaceAroundFieldsIsTrimmed) {
    auto parsed = read_csv_format("1, 2, 2^2, 2\n 0 , 1 \n");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ((parsed->array[0, 0]), 0);
    EXPECT_EQ((parsed->array[0, 1]), 1);
}

TEST(CsvFormat, EmptyFileIsRejected) {
    auto parsed = read_csv_format("");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(CsvFormat, TooFewHeaderFieldsIsRejected) {
    auto parsed = read_csv_format("2,2\n0,0\n0,0\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(CsvFormat, LevelSpecColumnCountMismatchIsRejected) {
    auto parsed = read_csv_format("2,3,2^2,2\n0,0,0\n0,0,0\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(CsvFormat, RowCountMismatchIsRejected) {
    auto parsed = read_csv_format("1,2,2^2,2\n0,0\n0,0\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::row_count_mismatch);
}

TEST(CsvFormat, ColumnCountMismatchIsRejected) {
    auto parsed = read_csv_format("1,3,2^3,2\n0,0\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::column_count_mismatch);
}

TEST(CsvFormat, OutOfRangeSymbolIsRejected) {
    auto parsed = read_csv_format("1,2,2^2,2\n0,5\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
}

TEST(CsvFormat, NonNumericSymbolIsRejected) {
    auto parsed = read_csv_format("1,2,2^2,2\n0,x\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
}

TEST(CsvFormat, ErrorsCiteThePhysicalFileLineNotTheLogicalRowIndex) {
    // A blank line between the header and the data row means the data
    // row's physical line number (3) differs from its logical row index
    // (0); the error must cite the real line.
    auto parsed = read_csv_format("1,2,2^2,2\n\n0,x\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
    EXPECT_NE(parsed.error().message.find("line 3"), std::string::npos) << parsed.error().message;
}

TEST(CsvFormat, InvalidSymbolErrorCitesTheFieldsColumn) {
    auto parsed = read_csv_format("1,2,2^2,2\n0, 5\n");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
    // "0, 5" - the "5" field's trimmed content starts at character 4.
    EXPECT_NE(parsed.error().message.find("column 4"), std::string::npos) << parsed.error().message;
}

} // namespace covarray
