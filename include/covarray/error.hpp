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
};

struct Error {
    ErrorCode code;
    std::string message;
};

} // namespace covarray
