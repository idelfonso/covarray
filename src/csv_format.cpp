#include "covarray/csv_format.hpp"

#include <cstdint>
#include <format>
#include <span>
#include <utility>
#include <vector>

#include "format_support.hpp"

namespace covarray {

using namespace format_support;

namespace {

// A field together with its 1-indexed column (character offset) in its
// line, for error messages that can point right at the offending field.
struct Field {
    std::size_t column;
    std::string_view text;
};

// Splits `line` on ',', trimming spaces/tabs around each field. Trailing
// commas never introduce an extra empty field. Each field's `column`
// points at its trimmed content (or at the field's start, if it's
// blank), not at the raw, untrimmed span.
[[nodiscard]] auto split_csv(std::string_view line) -> std::vector<Field> {
    std::vector<Field> fields;
    std::size_t start = 0;
    while (true) {
        const auto end = line.find(',', start);
        const auto raw =
            (end == std::string_view::npos) ? line.substr(start) : line.substr(start, end - start);
        const auto trim_start = raw.find_first_not_of(" \t");
        if (trim_start != std::string_view::npos) {
            const auto trim_end = raw.find_last_not_of(" \t");
            fields.push_back(
                {.column = start + trim_start + 1,
                 .text = raw.substr(trim_start, trim_end - trim_start + 1)}
            );
        } else {
            fields.push_back({.column = start + 1, .text = raw.substr(0, 0)});
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return fields;
}

// A line together with its 1-indexed physical line number in the file,
// for error messages that can point right at the offending line.
struct Line {
    std::size_t number;
    std::string_view text;
};

// Splits `text` into non-blank lines, each with its real file line
// number; blank lines (including a trailing one from a final newline)
// are dropped, never counted as content, but still advance the count so
// line numbers stay accurate. Hand-rolled rather than `std::views::split`
// because `line_number` has to advance in lockstep with the scan even for
// a dropped blank line -- a views pipeline would need its own counter
// zipped in anyway, so there's nothing to gain by routing through one.
[[nodiscard]] auto content_lines(std::string_view text) -> std::vector<Line> {
    std::vector<Line> lines;
    std::size_t pos = 0;
    std::size_t line_number = 1;
    while (pos <= text.size()) {
        const auto end = text.find('\n', pos);
        const auto raw =
            (end == std::string_view::npos) ? text.substr(pos) : text.substr(pos, end - pos);
        if (!is_blank(raw)) {
            lines.push_back({.number = line_number, .text = raw});
        }
        ++line_number;
        if (end == std::string_view::npos) {
            break;
        }
        pos = end + 1;
    }
    return lines;
}

struct HeaderLine {
    std::size_t rows;
    std::vector<std::uint8_t> levels; // one entry per column, in file order
    unsigned strength;
};

// Parses `N,k,<level-spec>,t`, expanding the level-spec (one or more
// `v^c` fields) into one level value per column. Error messages cite
// `line`'s real file line number (1-indexed), not a logical row index.
[[nodiscard]] auto parse_header_line(const Line& line) -> std::expected<HeaderLine, Error> {
    const auto fields = split_csv(line.text);
    if (fields.size() < 4) {
        return malformed(
            std::format("line {}: header line has too few fields: '{}'", line.number, line.text)
        );
    }

    const auto rows = parse_unsigned<std::size_t>(fields.front().text);
    if (!rows) {
        return malformed(
            std::format(
                "line {}, column {}: invalid row count '{}'", line.number, fields.front().column,
                fields.front().text
            )
        );
    }
    const auto columns = parse_unsigned<std::size_t>(fields[1].text);
    if (!columns) {
        return malformed(
            std::format(
                "line {}, column {}: invalid column count '{}'", line.number, fields[1].column,
                fields[1].text
            )
        );
    }
    const auto strength = parse_unsigned<unsigned>(fields.back().text);
    if (!strength) {
        return malformed(
            std::format(
                "line {}, column {}: invalid strength '{}'", line.number, fields.back().column,
                fields.back().text
            )
        );
    }

    std::vector<std::string_view> spec_tokens;
    spec_tokens.reserve(fields.size() - 3);
    for (const auto& field : std::span<const Field>(fields).subspan(2, fields.size() - 3)) {
        spec_tokens.push_back(field.text);
    }
    auto levels = parse_level_spec_tokens(spec_tokens, *columns);
    if (!levels) {
        return std::unexpected(levels.error());
    }
    if (levels->size() != *columns) {
        return malformed(
            std::format(
                "line {}: level spec covers {} columns, but the header line declares {}",
                line.number, levels->size(), *columns
            )
        );
    }

    return HeaderLine{.rows = *rows, .levels = std::move(*levels), .strength = *strength};
}

} // namespace

auto read_csv_format(std::string_view text) -> std::expected<ParsedCoveringArray, Error> {
    const auto lines = content_lines(text);
    if (lines.empty()) {
        return malformed("file has no header line");
    }

    auto header = parse_header_line(lines.front());
    if (!header) {
        return std::unexpected(header.error());
    }
    const auto rows = header->rows;
    const auto columns = header->levels.size();

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
        const auto& data_line = lines[r + 1];
        const auto fields = split_csv(data_line.text);
        if (fields.size() != columns) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::column_count_mismatch,
                    .message = std::format(
                        "line {}: row has {} values, expected {}", data_line.number, fields.size(),
                        columns
                    ),
                }
            );
        }
        for (std::size_t c = 0; c < columns; ++c) {
            const auto level = header->levels[c];
            const auto& field = fields[c];
            if (field.text == "-") {
                array.at(r, c) = level;
                continue;
            }
            const auto value = parse_unsigned<unsigned>(field.text);
            if (!value || *value > level) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::invalid_symbol,
                        .message = std::format(
                            "line {}, column {}: invalid symbol '{}' for level {}",
                            data_line.number, field.column, field.text, level
                        ),
                    }
                );
            }
            array.at(r, c) = static_cast<std::uint8_t>(*value);
        }
    }

    const auto order = decreasing_level_order(header->levels);
    std::vector<std::uint8_t> sorted_levels(columns);
    CoveringArray sorted_array(rows, columns);
    for (std::size_t new_c = 0; new_c < columns; ++new_c) {
        const auto old_c = order[new_c];
        sorted_levels[new_c] = header->levels[old_c];
        for (std::size_t r = 0; r < rows; ++r) {
            sorted_array.at(r, new_c) = array.at(r, old_c);
        }
    }

    auto spec = Spec::create(header->strength, std::move(sorted_levels));
    if (!spec) {
        return std::unexpected(spec.error());
    }
    return ParsedCoveringArray{.array = std::move(sorted_array), .spec = std::move(*spec)};
}

auto write_csv_format(const CoveringArray& array, const Spec& spec) -> std::string {
    std::string out;

    out += std::format("{},{}", array.rows(), array.columns());
    const auto levels = spec.levels();
    for (std::size_t i = 0; i < levels.size();) {
        std::size_t j = i + 1;
        while (j < levels.size() && levels[j] == levels[i]) {
            ++j;
        }
        out += std::format(",{}^{}", levels[i], j - i);
        i = j;
    }
    out += std::format(",{}\n", spec.strength());

    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            out +=
                (c == 0) ? std::format("{}", array.at(r, c)) : std::format(",{}", array.at(r, c));
        }
        out += '\n';
    }
    return out;
}

} // namespace covarray
