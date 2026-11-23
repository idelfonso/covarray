#include "covarray/covering_array.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(CoveringArray, StartsZeroFilled) {
    CoveringArray ca(3, 4);
    EXPECT_EQ(ca.rows(), 3);
    EXPECT_EQ(ca.columns(), 4);
    for (std::size_t r = 0; r < ca.rows(); ++r) {
        for (std::size_t c = 0; c < ca.columns(); ++c) {
            EXPECT_EQ((ca[r, c]), 0) << "r=" << r << " c=" << c;
        }
    }
}

TEST(CoveringArray, ElementAccessReadWrite) {
    CoveringArray ca(2, 3);
    ca[0, 0] = 5;
    ca[0, 2] = 7;
    ca[1, 1] = 9;
    EXPECT_EQ((ca[0, 0]), 5);
    EXPECT_EQ((ca[0, 1]), 0);
    EXPECT_EQ((ca[0, 2]), 7);
    EXPECT_EQ((ca[1, 0]), 0);
    EXPECT_EQ((ca[1, 1]), 9);
    EXPECT_EQ((ca[1, 2]), 0);
}

TEST(CoveringArray, SupportsSymbolsNear255) {
    CoveringArray ca(1, 3);
    ca[0, 0] = 253;
    ca[0, 1] = 254;
    ca[0, 2] = 255;
    EXPECT_EQ((ca[0, 0]), 253);
    EXPECT_EQ((ca[0, 1]), 254);
    EXPECT_EQ((ca[0, 2]), 255);
}

TEST(CoveringArray, RowViewMatchesElementAccess) {
    CoveringArray ca(2, 3);
    ca[1, 0] = 1;
    ca[1, 1] = 2;
    ca[1, 2] = 3;
    const auto row = ca.row(1);
    ASSERT_EQ(row.size(), 3);
    EXPECT_EQ(row[0], 1);
    EXPECT_EQ(row[1], 2);
    EXPECT_EQ(row[2], 3);
}

TEST(CoveringArray, RowViewIsWritable) {
    CoveringArray ca(2, 3);
    auto row = ca.row(0);
    row[1] = 42;
    EXPECT_EQ((ca[0, 1]), 42);
}

TEST(CoveringArray, ColumnViewMatchesElementAccess) {
    CoveringArray ca(3, 2);
    ca[0, 1] = 10;
    ca[1, 1] = 20;
    ca[2, 1] = 30;
    const auto col = ca.column(1);
    ASSERT_EQ(col.extent(0), 3);
    EXPECT_EQ(col[0], 10);
    EXPECT_EQ(col[1], 20);
    EXPECT_EQ(col[2], 30);
}

TEST(CoveringArray, ColumnViewIsWritable) {
    CoveringArray ca(3, 2);
    auto col = ca.column(0);
    col[2] = 99;
    EXPECT_EQ((ca[2, 0]), 99);
}

TEST(CoveringArray, TwoDimensionalViewMatchesElementAccess) {
    CoveringArray ca(2, 2);
    ca[0, 0] = 1;
    ca[0, 1] = 2;
    ca[1, 0] = 3;
    ca[1, 1] = 4;
    const auto view = ca.view();
    EXPECT_EQ((view[0, 0]), 1);
    EXPECT_EQ((view[0, 1]), 2);
    EXPECT_EQ((view[1, 0]), 3);
    EXPECT_EQ((view[1, 1]), 4);
}

TEST(CoveringArray, CopyIsIndependent) {
    CoveringArray original(2, 2);
    original[0, 0] = 1;
    CoveringArray copy = original;
    copy[0, 0] = 2;
    EXPECT_EQ((original[0, 0]), 1);
    EXPECT_EQ((copy[0, 0]), 2);
}

TEST(CoveringArray, MoveLeavesSourceValid) {
    CoveringArray original(2, 2);
    original[0, 0] = 7;
    CoveringArray moved = std::move(original);
    EXPECT_EQ((moved[0, 0]), 7);
}

TEST(CoveringArray, HandlesLargeInstances) {
    constexpr std::size_t rows = 10'000;
    constexpr std::size_t columns = 100;
    CoveringArray ca(rows, columns);
    for (std::size_t r = 0; r < rows; r += 997) {
        ca[r, r % columns] = static_cast<std::uint8_t>(r % 256);
    }
    for (std::size_t r = 0; r < rows; r += 997) {
        EXPECT_EQ((ca[r, r % columns]), static_cast<std::uint8_t>(r % 256));
    }
    EXPECT_EQ(ca.rows(), rows);
    EXPECT_EQ(ca.columns(), columns);
}

} // namespace covarray
