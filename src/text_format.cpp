#include "covarray/text_format.hpp"

#include <cctype>
#include <cstdint>
#include <format>
#include <span>
#include <vector>

#include "format_support.hpp"

namespace covarray {

using namespace format_support;

namespace {

// Python-style comment: '#' and everything after it, even after data on
// the same line.
[[nodiscard]] auto strip_hash_comment(std::string_view line) -> std::string_view {
    const auto pos = line.find('#');
    return pos == std::string_view::npos ? line : line.substr(0, pos);
}

// DIMACS-style comment: the first non-blank character is 'c'/'C',
// immediately followed by a blank. A word like "construct" is not a
// comment, since the character right after 'c' isn't blank.
[[nodiscard]] auto is_dimacs_comment(std::string_view line) -> bool {
    const auto start = line.find_first_not_of(" \t");
    if (start == std::string_view::npos) {
        return false;
    }
    if (line[start] != 'c' && line[start] != 'C') {
        return false;
    }
    return start + 1 < line.size() && (line[start + 1] == ' ' || line[start + 1] == '\t');
}

// Splits `text` into lines with comments stripped and blank lines
// dropped: every line this returns is real parameter/data content.
// A plain loop over `find('\n')`, not `std::views::split`, since each
// line still needs its own stripping (comment removal, then a blank
// check) before it's kept or dropped -- keeping the scan explicit avoids
// turning a single linear pass into a multi-stage views pipeline.
[[nodiscard]] auto content_lines(std::string_view text) -> std::vector<std::string_view> {
    std::vector<std::string_view> lines;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const auto end = text.find('\n', pos);
        const auto raw =
            (end == std::string_view::npos) ? text.substr(pos) : text.substr(pos, end - pos);
        if (!is_dimacs_comment(raw)) {
            if (const auto stripped = strip_hash_comment(raw); !is_blank(stripped)) {
                lines.push_back(stripped);
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
        pos = end + 1;
    }
    return lines;
}

[[nodiscard]] auto split_whitespace(std::string_view line) -> std::vector<std::string_view> {
    std::vector<std::string_view> tokens;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])) != 0) {
            ++i;
        }
        const auto start = i;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])) == 0) {
            ++i;
        }
        if (i > start) {
            tokens.push_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

struct ParameterLine {
    std::size_t rows;
    std::vector<std::uint8_t> levels; // one entry per column, in file order
    unsigned strength;
};

// Parses `N k <level-spec> t`, expanding the level-spec (one or more
// `v^c` tokens) into one level value per column.
[[nodiscard]] auto parse_parameter_line(std::string_view line)
    -> std::expected<ParameterLine, Error> {
    const auto tokens = split_whitespace(line);
    if (tokens.size() < 4) {
        return malformed(std::format("parameter line has too few fields: '{}'", line));
    }

    const auto rows = parse_unsigned<std::size_t>(tokens.front());
    if (!rows) {
        return malformed(std::format("invalid row count '{}'", tokens.front()));
    }
    const auto columns = parse_unsigned<std::size_t>(tokens[1]);
    if (!columns) {
        return malformed(std::format("invalid column count '{}'", tokens[1]));
    }
    const auto strength = parse_unsigned<unsigned>(tokens.back());
    if (!strength) {
        return malformed(std::format("invalid strength '{}'", tokens.back()));
    }

    const auto spec_tokens =
        std::span<const std::string_view>(tokens).subspan(2, tokens.size() - 3);
    auto levels = parse_level_spec_tokens(spec_tokens, *columns);
    if (!levels) {
        return std::unexpected(levels.error());
    }
    if (levels->size() != *columns) {
        return malformed(
            std::format(
                "level spec covers {} columns, but the parameter line declares {}", levels->size(),
                *columns
            )
        );
    }

    return ParameterLine{.rows = *rows, .levels = std::move(*levels), .strength = *strength};
}

} // namespace

auto read_text_format(std::string_view text) -> std::expected<ParsedCoveringArray, Error> {
    const auto lines = content_lines(text);
    if (lines.empty()) {
        return malformed("file has no parameter line");
    }

    auto parameters = parse_parameter_line(lines.front());
    if (!parameters) {
        return std::unexpected(parameters.error());
    }
    const auto rows = parameters->rows;
    const auto columns = parameters->levels.size();

    if (lines.size() - 1 != rows) {
        return std::unexpected(
            Error{
                .code = ErrorCode::row_count_mismatch,
                .message = std::format("expected {} data rows, found {}", rows, lines.size() - 1),
            }
        );
    }

    CoveringArray array(rows, columns);
    for (std::size_t r = 0; r < rows; ++r) {
        const auto tokens = split_whitespace(lines[r + 1]);
        if (tokens.size() != columns) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::column_count_mismatch,
                    .message =
                        std::format("row {} has {} values, expected {}", r, tokens.size(), columns),
                }
            );
        }
        for (std::size_t c = 0; c < columns; ++c) {
            const auto level = parameters->levels[c];
            if (tokens[c] == "-") {
                array.at(r, c) = level;
                continue;
            }
            const auto value = parse_unsigned<unsigned>(tokens[c]);
            if (!value || *value > level) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::invalid_symbol,
                        .message = std::format(
                            "row {} column {} has invalid symbol '{}' for level {}", r, c,
                            tokens[c], level
                        ),
                    }
                );
            }
            array.at(r, c) = static_cast<std::uint8_t>(*value);
        }
    }

    const auto order = decreasing_level_order(parameters->levels);
    std::vector<std::uint8_t> sorted_levels(columns);
    CoveringArray sorted_array(rows, columns);
    for (std::size_t new_c = 0; new_c < columns; ++new_c) {
        const auto old_c = order[new_c];
        sorted_levels[new_c] = parameters->levels[old_c];
        for (std::size_t r = 0; r < rows; ++r) {
            sorted_array.at(r, new_c) = array.at(r, old_c);
        }
    }

    auto spec = Spec::create(parameters->strength, std::move(sorted_levels));
    if (!spec) {
        return std::unexpected(spec.error());
    }
    return ParsedCoveringArray{.array = std::move(sorted_array), .spec = std::move(*spec)};
}

auto write_text_format(const CoveringArray& array, const Spec& spec) -> std::string {
    std::string out;

    out += std::format("{} {}", array.rows(), array.columns());
    const auto levels = spec.levels();
    for (std::size_t i = 0; i < levels.size();) {
        std::size_t j = i + 1;
        while (j < levels.size() && levels[j] == levels[i]) {
            ++j;
        }
        out += std::format(" {}^{}", levels[i], j - i);
        i = j;
    }
    out += std::format(" {}\n", spec.strength());

    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            out +=
                (c == 0) ? std::format("{}", array.at(r, c)) : std::format(" {}", array.at(r, c));
        }
        out += '\n';
    }
    return out;
}

} // namespace covarray
