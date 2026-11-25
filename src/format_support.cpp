#include "format_support.hpp"

#include <algorithm>
#include <format>

namespace covarray::format_support {

auto malformed(std::string message) -> std::unexpected<Error> {
    return std::unexpected(
        Error{.code = ErrorCode::malformed_parameter_line, .message = std::move(message)}
    );
}

auto parse_level_spec_tokens(std::span<const std::string_view> spec_tokens, std::size_t columns)
    -> std::expected<std::vector<std::uint8_t>, Error> {
    std::vector<std::uint8_t> levels;
    for (const auto spec_token : spec_tokens) {
        const auto caret = spec_token.find('^');
        if (caret == std::string_view::npos) {
            return malformed(std::format("malformed level spec '{}'", spec_token));
        }
        const auto level = parse_unsigned<unsigned>(spec_token.substr(0, caret));
        const auto count = parse_unsigned<std::size_t>(spec_token.substr(caret + 1));
        if (!level || !count || *level > 255) {
            return malformed(std::format("malformed level spec '{}'", spec_token));
        }
        if (*count > columns - levels.size()) {
            return malformed(
                std::format("level spec covers more than the declared {} columns", columns)
            );
        }
        levels.insert(levels.end(), *count, static_cast<std::uint8_t>(*level));
    }
    return levels;
}

auto is_blank(std::string_view line) -> bool {
    return line.find_first_not_of(" \t") == std::string_view::npos;
}

auto decreasing_level_order(std::span<const std::uint8_t> levels) -> std::vector<std::size_t> {
    std::vector<std::size_t> order(levels.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::ranges::stable_sort(order, [&](std::size_t a, std::size_t b) -> bool {
        return levels[a] > levels[b];
    });
    return order;
}

} // namespace covarray::format_support
