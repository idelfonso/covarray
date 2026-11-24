#pragma once

#include <cstdint>
#include <string>

namespace covarray {

enum class ErrorCode : std::uint8_t {
    invalid_strength,
    invalid_level,
    empty_columns,
    forbidden_tuple_out_of_range,
    seed_row_mismatch,
    malformed_parameter_line,
    invalid_symbol,
    row_count_mismatch,
    column_count_mismatch,
};

struct Error {
    ErrorCode code;
    std::string message;
};

} // namespace covarray
