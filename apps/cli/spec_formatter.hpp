#pragma once

#include "covarray/spec.hpp"

#include <format>

// A std::formatter specialization for covarray::Spec, so the CLI's
// output goes through std::format/std::print instead of ad hoc string
// concatenation. Printed as "k=<columns> t=<strength> levels=[v0,v1,...]"
// -- deliberately not N (Spec never carries it; see spec.hpp), so a
// caller that also wants the row count formats it alongside this.
template <> struct std::formatter<covarray::Spec> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const covarray::Spec& spec, std::format_context& ctx) {
        auto out = std::format_to(ctx.out(), "k={} t={} levels=[", spec.columns(), spec.strength());
        const auto levels = spec.levels();
        for (std::size_t i = 0; i < levels.size(); ++i) {
            out = (i == 0) ? std::format_to(out, "{}", levels[i])
                           : std::format_to(out, ",{}", levels[i]);
        }
        return std::format_to(out, "]");
    }
};
