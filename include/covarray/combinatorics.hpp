#pragma once

#include <cstdint>

namespace covarray {

// Multiplicative formula: result stays an exact integer after each step,
// so this avoids the overflow a naive n!/(k!(n-k)!) would hit much sooner.
[[nodiscard]] constexpr auto binomial(unsigned n, unsigned k) noexcept -> std::uint64_t {
    if (k > n) {
        return 0;
    }
    if (k > n - k) {
        k = n - k;
    }

    std::uint64_t result = 1;
    for (unsigned i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }
    return result;
}

} // namespace covarray
