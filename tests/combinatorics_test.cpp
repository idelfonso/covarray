#include "covarray/combinatorics.hpp"

#include <gtest/gtest.h>

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
            EXPECT_EQ(binomial(n, k), binomial(n, n - k))
                << "n=" << n << " k=" << k;
        }
    }
}

TEST(Binomial, PascalsRule) {
    for (unsigned n = 1; n <= 20; ++n) {
        for (unsigned k = 1; k < n; ++k) {
            EXPECT_EQ(
                binomial(n, k), binomial(n - 1, k - 1) + binomial(n - 1, k)
            ) << "n="
              << n << " k=" << k;
        }
    }
}

TEST(Binomial, KGreaterThanNIsZero) {
    EXPECT_EQ(binomial(3, 5), 0);
    EXPECT_EQ(binomial(0, 1), 0);
}

} // namespace covarray
