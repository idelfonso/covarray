#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace covarray {

// Multiplicative formula: result stays an exact integer after each step,
// so this avoids the overflow a naive n!/(k!(n-k)!) would hit much sooner.
[[nodiscard]] constexpr auto binomial(unsigned n, unsigned k) noexcept -> std::uint64_t {
    if (k > n) {
        return 0;
    }
    k = std::min(k, n - k);

    std::uint64_t result = 1;
    for (unsigned i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }
    return result;
}

// A bijection between t-combinations of `{0, ..., n-1}` (strictly
// increasing index sequences) and their rank in `[0, binomial(n, t))`
// under lexicographic order (comparing element-wise from the smallest
// index — the same order `next_combination` advances through). This is
// the addressing scheme the parallel verifier (Thu Nov 26) uses to split
// the space of t-sets into equal ranges by rank, rather than
// materializing all of them.

// `combination` must be strictly increasing with every element `< n`;
// its rank among all `combination.size()`-combinations of `{0, ..., n-1}`
// in lexicographic order.
[[nodiscard]] constexpr auto
rank_combination(std::span<const std::size_t> combination, std::size_t n) noexcept
    -> std::uint64_t {
    const auto t = combination.size();
    // Count combinations that sort before `combination`: for each position
    // `i`, every smaller choice at that position (with the same prefix)
    // contributes binomial(n - 1 - v, t - i) ways to fill the rest.
    std::uint64_t before = 0;
    for (std::size_t i = 0; i < t; ++i) {
        before +=
            binomial(static_cast<unsigned>(n - 1 - combination[i]), static_cast<unsigned>(t - i));
    }
    return binomial(static_cast<unsigned>(n), static_cast<unsigned>(t)) - 1 - before;
}

// Inverse of `rank_combination`: the `rank`-th t-combination of
// `{0, ..., n-1}` in lexicographic order, strictly increasing.
[[nodiscard]] inline auto unrank_combination(std::uint64_t rank, std::size_t n, std::size_t t)
    -> std::vector<std::size_t> {
    std::vector<std::size_t> combination(t);
    std::size_t v = 0;
    for (std::size_t i = 0; i < t; ++i) {
        // Greedily grow `v` until fixing combination[i] = v leaves exactly
        // `rank` combinations before it among those sharing this prefix.
        for (;;) {
            const auto count =
                binomial(static_cast<unsigned>(n - 1 - v), static_cast<unsigned>(t - 1 - i));
            if (rank < count) {
                combination[i] = v;
                ++v;
                break;
            }
            rank -= count;
            ++v;
        }
    }
    return combination;
}

// Advances `combination` (strictly increasing indices into `{0, ..., n-1}`)
// to its lexicographic successor in place. Returns `false` once
// `combination` was already the last one, leaving it unchanged. The cheap,
// O(t)-amortized path for a verifier looping over every t-set in order;
// `rank_combination`/`unrank_combination` stay reserved for random access
// (e.g. splitting the range across threads).
[[nodiscard]] inline auto
next_combination(std::span<std::size_t> combination, std::size_t n) noexcept -> bool {
    const std::size_t t = combination.size();
    if (t == 0) {
        return false;
    }
    std::size_t i = t;
    while (i-- > 0) {
        if (combination[i] + 1 < n - (t - 1 - i)) {
            ++combination[i];
            for (std::size_t j = i + 1; j < t; ++j) {
                combination[j] = combination[j - 1] + 1;
            }
            return true;
        }
    }
    return false;
}

// Calls `fn(combination)` once for every t-combination of `{0, ..., n-1}`,
// in lexicographic order, starting from `{0, 1, ..., t-1}`.
template <typename F> void for_each_combination(std::size_t n, std::size_t t, F&& fn) {
    if (t > n) {
        return;
    }
    std::vector<std::size_t> combination(t);
    for (std::size_t i = 0; i < t; ++i) {
        combination[i] = i;
    }
    do {
        std::forward<F>(fn)(std::span<const std::size_t>(combination));
    } while (next_combination(combination, n));
}

// Mixed-radix encoding of a value tuple, one digit per column, each with
// its own radix (`levels[i]`, i.e. that column's symbol count). This is the
// addressing scheme the index verifier (Tue Nov 24, "v^t bitset per
// t-set") uses, generalized to mixed levels since `Spec::levels()` is
// already per-column rather than a single shared `v`.

// `values[i] < levels[i]` for every `i`. Most significant digit first
// (`values[0]`), matching a t-set's column order.
[[nodiscard]] constexpr auto
rank_tuple(std::span<const std::uint8_t> levels, std::span<const std::uint8_t> values) noexcept
    -> std::uint64_t {
    std::uint64_t index = 0;
    for (std::size_t i = 0; i < levels.size(); ++i) {
        index = (index * levels[i]) + values[i];
    }
    return index;
}

// Inverse of `rank_tuple`: the value tuple at `index` for the given
// per-column `levels`.
[[nodiscard]] inline auto unrank_tuple(std::uint64_t index, std::span<const std::uint8_t> levels)
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> values(levels.size());
    for (std::size_t i = levels.size(); i-- > 0;) {
        values[i] = static_cast<std::uint8_t>(index % levels[i]);
        index /= levels[i];
    }
    return values;
}

} // namespace covarray
