#include "covarray/covering_array.hpp"
#include "covarray/json_format.hpp"
#include "covarray/spec.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(JsonFormat, UniformRoundTrip) {
    auto spec = Spec::create(2, {3, 3, 3});
    ASSERT_TRUE(spec.has_value());

    CoveringArray array(4, 3);
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            array[r, c] = static_cast<std::uint8_t>((r + c) % 3);
        }
    }

    const auto text = write_json_format(array, *spec);
    auto parsed = read_json_format(text);
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

TEST(JsonFormat, WriteThenReadThenWriteIsIdempotent) {
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

    const auto text = write_json_format(array, *spec);
    auto parsed = read_json_format(text);
    ASSERT_TRUE(parsed.has_value());

    const auto text_again = write_json_format(parsed->array, parsed->spec);
    EXPECT_EQ(text_again, text);
}

TEST(JsonFormat, ColumnOrderIsPreservedNotReordered) {
    // Unlike text/CSV, JSON keeps whatever column order the document
    // gives it (no decreasing-level canonicalization).
    const auto parsed =
        read_json_format(R"({"n":1,"k":3,"t":2,"levels":[2,4,3],"rows":[[1,4,2]]})");
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

TEST(JsonFormat, WildcardIsNumericOnly) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"t":2,"levels":[3,2],"rows":[[3,1]]})");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ((parsed->array[0, 0]), 3);
    EXPECT_EQ((parsed->array[0, 1]), 1);
}

TEST(JsonFormat, InvalidJsonIsRejected) {
    auto parsed = read_json_format("{not json");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(JsonFormat, SyntaxErrorMessageCitesLineAndColumn) {
    // nlohmann computes line/column for a syntax error itself; this locks
    // in that it actually reaches Error::message, not just that parsing
    // fails.
    auto parsed = read_json_format("{\n  \"n\": 1,\n  \"k\": tru\n}");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
    EXPECT_NE(parsed.error().message.find("line"), std::string::npos) << parsed.error().message;
    EXPECT_NE(parsed.error().message.find("column"), std::string::npos) << parsed.error().message;
}

TEST(JsonFormat, NonObjectTopLevelIsRejected) {
    auto parsed = read_json_format("[1,2,3]");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(JsonFormat, MissingFieldIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"levels":[2,2],"rows":[[0,0]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(JsonFormat, LevelsLengthMismatchIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":3,"t":2,"levels":[2,2],"rows":[[0,0,0]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::malformed_parameter_line);
}

TEST(JsonFormat, RowCountMismatchIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"t":2,"levels":[2,2],"rows":[[0,0],[1,1]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::row_count_mismatch);
}

TEST(JsonFormat, ColumnCountMismatchIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"t":2,"levels":[2,2],"rows":[[0]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::column_count_mismatch);
}

TEST(JsonFormat, OutOfRangeSymbolIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"t":2,"levels":[2,2],"rows":[[0,5]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
}

TEST(JsonFormat, NonNumericSymbolIsRejected) {
    auto parsed = read_json_format(R"({"n":1,"k":2,"t":2,"levels":[2,2],"rows":[[0,"x"]]})");
    ASSERT_FALSE(parsed.has_value());
    EXPECT_EQ(parsed.error().code, ErrorCode::invalid_symbol);
}

} // namespace covarray
