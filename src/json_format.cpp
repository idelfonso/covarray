#include "covarray/json_format.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <format>
#include <span>
#include <vector>

#include "format_support.hpp"

namespace covarray {

using namespace format_support;

namespace {

[[nodiscard]] auto require_unsigned_field(const nlohmann::json& doc, const std::string& key)
    -> std::expected<std::uint64_t, Error> {
    if (!doc.contains(key) || !doc.at(key).is_number_unsigned()) {
        return malformed(std::format("missing or non-integer '{}' field", key));
    }
    return doc.at(key).get<std::uint64_t>();
}

[[nodiscard]] auto require_array_field(const nlohmann::json& doc, const std::string& key)
    -> std::expected<const nlohmann::json*, Error> {
    if (!doc.contains(key) || !doc.at(key).is_array()) {
        return malformed(std::format("missing or non-array '{}' field", key));
    }
    return &doc.at(key);
}

// Fills `array` (already sized `rows` x `levels.size()`) from the JSON
// `rows_json` array, checking each row's length and each cell's range
// against `levels`.
[[nodiscard]] auto fill_rows_from_json(
    const nlohmann::json& rows_json, std::span<const std::uint8_t> levels, std::size_t rows,
    CoveringArray& array
) -> std::expected<void, Error> {
    const auto columns = levels.size();
    for (std::size_t r = 0; r < rows; ++r) {
        const auto& row = rows_json.at(r);
        if (!row.is_array() || row.size() != columns) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::column_count_mismatch,
                    .message = std::format(
                        "row {} has {} values, expected {}", r,
                        row.is_array() ? row.size() : std::size_t{0}, columns
                    ),
                }
            );
        }
        for (std::size_t c = 0; c < columns; ++c) {
            const auto& cell = row.at(c);
            if (!cell.is_number_unsigned() || cell.get<std::uint64_t>() > levels[c]) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::invalid_symbol,
                        .message = std::format(
                            "row {} column {} has invalid symbol '{}' for level {}", r, c,
                            cell.dump(), levels[c]
                        ),
                    }
                );
            }
            array.at(r, c) = static_cast<std::uint8_t>(cell.get<unsigned>());
        }
    }
    return {};
}

} // namespace

auto read_json_format(std::string_view text) -> std::expected<ParsedCoveringArray, Error> {
    try {
        nlohmann::json doc;
        try {
            doc = nlohmann::json::parse(text);
        } catch (const nlohmann::json::parse_error& e) {
            return malformed(std::format("invalid JSON: {}", e.what()));
        }
        if (!doc.is_object()) {
            return malformed("top-level JSON value must be an object");
        }

        const auto rows = require_unsigned_field(doc, "n");
        if (!rows) {
            return std::unexpected(rows.error());
        }
        const auto declared_columns = require_unsigned_field(doc, "k");
        if (!declared_columns) {
            return std::unexpected(declared_columns.error());
        }
        const auto strength = require_unsigned_field(doc, "t");
        if (!strength) {
            return std::unexpected(strength.error());
        }

        const auto levels_json = require_array_field(doc, "levels");
        if (!levels_json) {
            return std::unexpected(levels_json.error());
        }
        if ((*levels_json)->size() != *declared_columns) {
            return malformed(
                std::format(
                    "'levels' has {} entries, but 'k' declares {}", (*levels_json)->size(),
                    *declared_columns
                )
            );
        }
        std::vector<std::uint8_t> levels;
        levels.reserve((*levels_json)->size());
        for (const auto& level : **levels_json) {
            if (!level.is_number_unsigned() || level.get<std::uint64_t>() > 255) {
                return malformed(std::format("invalid level '{}'", level.dump()));
            }
            levels.push_back(static_cast<std::uint8_t>(level.get<unsigned>()));
        }
        const auto columns = levels.size();

        const auto rows_json = require_array_field(doc, "rows");
        if (!rows_json) {
            return std::unexpected(rows_json.error());
        }
        if ((*rows_json)->size() != *rows) {
            return std::unexpected(
                Error{
                    .code = ErrorCode::row_count_mismatch,
                    .message =
                        std::format("expected {} data rows, found {}", *rows, (*rows_json)->size()),
                }
            );
        }

        CoveringArray array(*rows, columns);
        if (auto filled = fill_rows_from_json(**rows_json, levels, *rows, array); !filled) {
            return std::unexpected(filled.error());
        }

        auto spec = Spec::create(static_cast<unsigned>(*strength), std::move(levels));
        if (!spec) {
            return std::unexpected(spec.error());
        }
        return ParsedCoveringArray{.array = std::move(array), .spec = std::move(*spec)};
    } catch (const nlohmann::json::exception& e) {
        // Safety net: every access above is type-checked first, so this
        // should be unreachable, but a JSON document is untrusted input
        // and no exception may cross this function's boundary (ADR-003).
        return malformed(std::format("invalid JSON document: {}", e.what()));
    }
}

auto write_json_format(const CoveringArray& array, const Spec& spec) -> std::string {
    nlohmann::json doc;
    doc["n"] = array.rows();
    doc["k"] = array.columns();
    doc["t"] = spec.strength();
    doc["levels"] = std::vector<unsigned>(spec.levels().begin(), spec.levels().end());

    std::vector<std::vector<unsigned>> rows(array.rows(), std::vector<unsigned>(array.columns()));
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            rows[r][c] = array.at(r, c);
        }
    }
    doc["rows"] = std::move(rows);

    return doc.dump();
}

} // namespace covarray
