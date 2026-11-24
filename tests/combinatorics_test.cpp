#include "covarray/combinatorics.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace covarray {

static_assert(binomial(0, 0) == 1);
static_assert(binomial(5, 0) == 1);
static_assert(binomial(5, 5) == 1);
static_assert(binomial(5, 2) == 10);
static_assert(binomial(10, 3) == 120);
static_assert(binomial(3, 5) == 0);
static_assert(binomial(34, 17) == 2'333'606'220);

TEST(Binomial, BaseCases) {
    EXPECT_EQ(binomial(0, 0), 1);
    EXPECT_EQ(binomial(5, 0), 1);
    EXPECT_EQ(binomial(5, 5), 1);
}

TEST(Binomial, Symmetric) {
    for (unsigned n = 0; n <= 20; ++n) {
        for (unsigned k = 0; k <= n; ++k) {
            EXPECT_EQ(binomial(n, k), binomial(n, n - k)) << "n=" << n << " k=" << k;
        }
    }
}

TEST(Binomial, PascalsRule) {
    for (unsigned n = 1; n <= 20; ++n) {
        for (unsigned k = 1; k < n; ++k) {
            EXPECT_EQ(binomial(n, k), binomial(n - 1, k - 1) + binomial(n - 1, k))
                << "n=" << n << " k=" << k;
        }
    }
}

TEST(Binomial, KGreaterThanNIsZero) {
    EXPECT_EQ(binomial(3, 5), 0);
    EXPECT_EQ(binomial(0, 1), 0);
}

namespace {
// `(n, t)` pairs small enough to exhaustively check every rank below.
constexpr std::array<std::pair<std::size_t, std::size_t>, 5> kCombinationShapes{
    {{5, 0}, {5, 1}, {5, 3}, {5, 5}, {8, 4}},
};
} // namespace

TEST(CombinationRanking, UnrankThenRankIsIdentity) {
    for (const auto& [n, t] : kCombinationShapes) {
        for (std::uint64_t rank = 0;
             rank < binomial(static_cast<unsigned>(n), static_cast<unsigned>(t)); ++rank) {
            const auto combination = unrank_combination(rank, n, t);
            ASSERT_EQ(combination.size(), t);
            EXPECT_EQ(rank_combination(combination, n), rank)
                << "n=" << n << " t=" << t << " rank=" << rank;
        }
    }
}

TEST(CombinationRanking, UnrankProducesStrictlyIncreasingIndicesBelowN) {
    for (const auto& [n, t] : kCombinationShapes) {
        const auto total = binomial(static_cast<unsigned>(n), static_cast<unsigned>(t));
        for (std::uint64_t rank = 0; rank < total; ++rank) {
            const auto combination = unrank_combination(rank, n, t);
            for (std::size_t i = 0; i < combination.size(); ++i) {
                EXPECT_LT(combination[i], n);
                if (i > 0) {
                    EXPECT_LT(combination[i - 1], combination[i]);
                }
            }
        }
    }
}

TEST(CombinationIteration, VisitsEveryCombinationExactlyOnceInRankOrder) {
    for (const auto& [n, t] : kCombinationShapes) {
        const auto total = binomial(static_cast<unsigned>(n), static_cast<unsigned>(t));
        std::uint64_t expected_rank = 0;
        std::uint64_t visits = 0;
        for_each_combination(n, t, [&](std::span<const std::size_t> combination) -> void {
            EXPECT_EQ(rank_combination(combination, n), expected_rank) << "n=" << n << " t=" << t;
            ++expected_rank;
            ++visits;
        });
        EXPECT_EQ(visits, total) << "n=" << n << " t=" << t;
    }
}

TEST(CombinationIteration, EmptyWhenTExceedsN) {
    std::uint64_t visits = 0;
    for_each_combination(3, 5, [&](std::span<const std::size_t>) -> void {
        ++visits;
    });
    EXPECT_EQ(visits, 0);
}

namespace {
// Per-column level sets small enough to exhaustively check every index.
const std::vector<std::vector<std::uint8_t>> kTupleLevelSets{{2, 2, 2}, {3, 4}, {5, 2, 7, 3}, {9}};
} // namespace

TEST(TupleRanking, UnrankThenRankIsIdentity) {
    for (const auto& levels : kTupleLevelSets) {
        std::uint64_t total = 1;
        for (const auto level : levels) {
            total *= level;
        }
        for (std::uint64_t index = 0; index < total; ++index) {
            const auto values = unrank_tuple(index, levels);
            ASSERT_EQ(values.size(), levels.size());
            EXPECT_EQ(rank_tuple(levels, values), index);
        }
    }
}

TEST(TupleRanking, UnrankStaysWithinEachColumnsLevel) {
    for (const auto& levels : kTupleLevelSets) {
        std::uint64_t total = 1;
        for (const auto level : levels) {
            total *= level;
        }
        for (std::uint64_t index = 0; index < total; ++index) {
            const auto values = unrank_tuple(index, levels);
            for (std::size_t i = 0; i < levels.size(); ++i) {
                EXPECT_LT(values[i], levels[i]);
            }
        }
    }
}

} // namespace covarray
