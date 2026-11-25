#include "covarray/text_format.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

// Exercises the text parser directly against arbitrary bytes: the only
// contract it needs to keep is "either returns a result or an Error",
// never UB/a crash, on any input -- including ones no file-format rule
// would call well-formed.
extern "C" auto LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) -> int {
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    [[maybe_unused]] const auto result = covarray::read_text_format(text);
    return 0;
}
