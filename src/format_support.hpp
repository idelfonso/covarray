#pragma once

#include "covarray/error.hpp"

#include <charconv>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Small helpers shared by the on-disk format readers/writers
// (text_format.cpp, csv_format.cpp, json_format.cpp, binary_format.cpp).
// Not a public header: nothing under include/covarray/ depends on it, and
// it isn't installed.
namespace covarray::format_support {

// For an unsigned `T`, `std::from_chars` doesn't accept a leading `-` at
// all -- the standard only permits that for a signed `T` -- so a token
// like "-4" consumes zero characters and fails here, rather than parsing
// and wrapping around to a huge unsigned value. This is what protects
// every numeric header field (N, k, t, and a level-spec's `v`/`c`) across
// every file format from negative numbers, with no separate check needed.
template <class T> [[nodiscard]] auto parse_unsigned(std::string_view token) -> std::optional<T> {
    T value{};
    const auto* end = token.data() + token.size();
    const auto result = std::from_chars(token.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] auto malformed(std::string message) -> std::unexpected<Error>;

// True for a line with nothing but spaces/tabs (or nothing at all).
[[nodiscard]] auto is_blank(std::string_view line) -> bool;

// Parses one or more `v^c` tokens (a mixed- or uniform-level spec) into
// one level value per column, in the tokens' own order. Each token's `v`
// must fit in a `std::uint8_t`; the running total of `c`s is bounded by
// `columns` as each token is parsed (rejected early as malformed, not
// grown into first) so a token like `2^9999999999999993` fails with an
// ordinary error instead of attempting a multi-exabyte allocation. The
// caller still checks the result covers the full expected column count.
[[nodiscard]] auto
parse_level_spec_tokens(std::span<const std::string_view> spec_tokens, std::size_t columns)
    -> std::expected<std::vector<std::uint8_t>, Error>;

// Stable order that sorts column indices by decreasing level.
[[nodiscard]] auto decreasing_level_order(std::span<const std::uint8_t> levels)
    -> std::vector<std::size_t>;

} // namespace covarray::format_support
