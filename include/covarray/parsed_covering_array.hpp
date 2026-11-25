#pragma once

#include "covarray/covering_array.hpp"
#include "covarray/spec.hpp"

namespace covarray {

// A covering array read from one of the on-disk formats (text, CSV,
// JSON, binary), together with the strength and per-column levels that
// format's header/metadata declared.
struct ParsedCoveringArray {
    CoveringArray array;
    Spec spec;
};

} // namespace covarray
