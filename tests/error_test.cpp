#include "covarray/error.hpp"

#include <gtest/gtest.h>

namespace covarray {

TEST(Error, HoldsCodeAndMessage) {
    const Error error{
        .code = ErrorCode::invalid_strength,
        .message = "strength must be at least 2",
    };
    EXPECT_EQ(error.code, ErrorCode::invalid_strength);
    EXPECT_EQ(error.message, "strength must be at least 2");
}

} // namespace covarray
