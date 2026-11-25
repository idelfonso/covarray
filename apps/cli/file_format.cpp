#include "file_format.hpp"

#include "covarray/binary_format.hpp"
#include "covarray/csv_format.hpp"
#include "covarray/json_format.hpp"
#include "covarray/text_format.hpp"

#include <cctype>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>

namespace covarray::cli {

namespace {

[[nodiscard]] auto extension_of(const std::string& path) -> std::string {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string extension = path.substr(dot + 1);
    for (auto& c : extension) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return extension;
}

[[nodiscard]] auto read_file_bytes(const std::string& path) -> std::optional<std::string> {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

auto read_covering_array_file(const std::string& path)
    -> std::expected<ParsedCoveringArray, std::string> {
    const auto extension = extension_of(path);
    if (extension != "txt" && extension != "csv" && extension != "json" && extension != "bin") {
        return std::unexpected(
            std::format(
                "unrecognized file extension '.{}' (expected .txt, .csv, .json, or .bin): {}",
                extension, path
            )
        );
    }

    const auto bytes = read_file_bytes(path);
    if (!bytes.has_value()) {
        return std::unexpected(std::format("cannot read file: {}", path));
    }

    auto parsed = [&] -> std::expected<ParsedCoveringArray, Error> {
        if (extension == "txt") {
            return read_text_format(*bytes);
        }
        if (extension == "csv") {
            return read_csv_format(*bytes);
        }
        if (extension == "json") {
            return read_json_format(*bytes);
        }
        return read_binary_format(*bytes);
    }();

    if (!parsed.has_value()) {
        return std::unexpected(parsed.error().message);
    }
    return *std::move(parsed);
}

} // namespace covarray::cli
