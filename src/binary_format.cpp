#include "covarray/binary_format.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <format>
#include <vector>

#include "format_support.hpp"

namespace covarray {

using namespace format_support;

namespace {

constexpr std::array<char, 4> kMagic{'C', 'V', 'A', 'R'};
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kHeaderSize = 4 + 1 + 8 + 8 + 4; // magic + version + N + k + t

// Little-endian on disk regardless of host endianness; byte-swapping is
// its own inverse, so the same helper serves both directions.
// `std::bit_cast` reinterprets the byte array's bits as `T` directly
// (legal here since both are trivially copyable and the same size) —
// unlike `reinterpret_cast`, it doesn't alias the same storage as two
// different types, so there's no strict-aliasing UB to worry about.
template <class T>
[[nodiscard]] auto read_little_endian(std::string_view bytes, std::size_t offset) -> T {
    std::array<char, sizeof(T)> buffer{};
    std::ranges::copy(bytes.substr(offset, sizeof(T)), buffer.begin());
    auto value = std::bit_cast<T>(buffer);
    if constexpr (std::endian::native != std::endian::little) {
        value = std::byteswap(value);
    }
    return value;
}

template <class T> auto append_little_endian(std::string& out, T value) -> void {
    if constexpr (std::endian::native != std::endian::little) {
        value = std::byteswap(value);
    }
    const auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
    out.append(bytes.data(), bytes.size());
}

} // namespace

auto read_binary_format(std::string_view bytes) -> std::expected<ParsedCoveringArray, Error> {
    if (bytes.size() < kHeaderSize) {
        return malformed(
            std::format(
                "binary file is {} bytes, shorter than the {}-byte fixed header", bytes.size(),
                kHeaderSize
            )
        );
    }
    if (!std::ranges::equal(bytes.substr(0, kMagic.size()), kMagic)) {
        return malformed("bad magic bytes: not a covarray binary file");
    }
    const auto version = static_cast<std::uint8_t>(bytes[kMagic.size()]);
    if (version != kVersion) {
        return malformed(std::format("unsupported binary format version {}", version));
    }

    const auto rows = read_little_endian<std::uint64_t>(bytes, 5);
    const auto columns = read_little_endian<std::uint64_t>(bytes, 13);
    const auto strength = read_little_endian<std::uint32_t>(bytes, 21);

    // `rows` and `columns` are untrusted uint64_t values straight from the
    // file; this assumes `rows * columns` doesn't overflow uint64_t rather
    // than checking it, unlike the text format's level-spec count (see
    // format_support.cpp) which was fixed to bound its own untrusted count
    // after the fuzzer found an allocation-size crash there. This reader
    // has no fuzz target yet, so that assumption hasn't been exercised.
    const auto levels_offset = kHeaderSize;
    const auto data_offset = levels_offset + columns;
    const auto expected_size = data_offset + (rows * columns);
    if (bytes.size() != expected_size) {
        return malformed(
            std::format(
                "expected {} bytes for N={}, k={}, found {}", expected_size, rows, columns,
                bytes.size()
            )
        );
    }

    std::vector<std::uint8_t> levels(columns);
    for (std::uint64_t c = 0; c < columns; ++c) {
        levels[c] = static_cast<std::uint8_t>(bytes[levels_offset + c]);
    }

    CoveringArray array(static_cast<std::size_t>(rows), static_cast<std::size_t>(columns));
    for (std::uint64_t r = 0; r < rows; ++r) {
        for (std::uint64_t c = 0; c < columns; ++c) {
            const auto cell = static_cast<std::uint8_t>(bytes[data_offset + (r * columns) + c]);
            if (cell > levels[c]) {
                return std::unexpected(
                    Error{
                        .code = ErrorCode::invalid_symbol,
                        .message = std::format(
                            "row {} column {} has invalid symbol {} for level {}", r, c, cell,
                            levels[c]
                        ),
                    }
                );
            }
            array.at(static_cast<std::size_t>(r), static_cast<std::size_t>(c)) = cell;
        }
    }

    auto spec = Spec::create(static_cast<unsigned>(strength), std::move(levels));
    if (!spec) {
        return std::unexpected(spec.error());
    }
    return ParsedCoveringArray{.array = std::move(array), .spec = std::move(*spec)};
}

auto write_binary_format(const CoveringArray& array, const Spec& spec) -> std::string {
    std::string out;
    out.append(kMagic.data(), kMagic.size());
    out.push_back(static_cast<char>(kVersion));
    append_little_endian<std::uint64_t>(out, array.rows());
    append_little_endian<std::uint64_t>(out, array.columns());
    append_little_endian<std::uint32_t>(out, spec.strength());

    for (const auto level : spec.levels()) {
        out.push_back(static_cast<char>(level));
    }
    for (std::size_t r = 0; r < array.rows(); ++r) {
        for (std::size_t c = 0; c < array.columns(); ++c) {
            out.push_back(static_cast<char>(array.at(r, c)));
        }
    }
    return out;
}

} // namespace covarray
