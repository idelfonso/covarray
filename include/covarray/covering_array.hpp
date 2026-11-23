#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mdspan>
#include <span>
#include <vector>

namespace covarray {

// Owning storage for a covering array's cells. Symbol type is fixed to
// `uint8_t`, not generic: v is at most 255 in practice, so a single byte
// per cell always suffices, and fixing the type keeps memory down for
// arrays with large N (rows) and k (columns). Storage is row-major; see
// docs/design/core-types.md for why, and for the deferred column-major
// consideration (verification only, pending a benchmark).
//
// `CoveringArray` has value semantics (copyable and movable, like
// `std::vector`) and does no validation of its own: it's raw storage, not
// a description of what the array should look like. That's `Spec`'s job.
class CoveringArray {
  public:
    using Extents2D = std::dextents<std::size_t, 2>;
    using Extents1D = std::dextents<std::size_t, 1>;

    using View = std::mdspan<std::uint8_t, Extents2D>;
    using ConstView = std::mdspan<const std::uint8_t, Extents2D>;
    using ColumnView = std::mdspan<std::uint8_t, Extents1D, std::layout_stride>;
    using ConstColumnView = std::mdspan<const std::uint8_t, Extents1D, std::layout_stride>;

    // Every cell starts at 0.
    CoveringArray(std::size_t rows, std::size_t columns)
        : rows_(rows), columns_(columns), data_(rows * columns, std::uint8_t{0}) {}

    [[nodiscard]] auto rows() const noexcept -> std::size_t {
        return rows_;
    }

    [[nodiscard]] auto columns() const noexcept -> std::size_t {
        return columns_;
    }

    [[nodiscard]] auto operator[](std::size_t row, std::size_t column) noexcept -> std::uint8_t& {
        return data_[(row * columns_) + column];
    }

    [[nodiscard]] auto operator[](std::size_t row, std::size_t column) const noexcept
        -> std::uint8_t {
        return data_[(row * columns_) + column];
    }

    [[nodiscard]] auto view() noexcept -> View {
        return View(data_.data(), rows_, columns_);
    }

    [[nodiscard]] auto view() const noexcept -> ConstView {
        return ConstView(data_.data(), rows_, columns_);
    }

    // A contiguous view of one row's `columns()` values.
    [[nodiscard]] auto row(std::size_t index) noexcept -> std::span<std::uint8_t> {
        return std::span(data_).subspan(index * columns_, columns_);
    }

    [[nodiscard]] auto row(std::size_t index) const noexcept -> std::span<const std::uint8_t> {
        return std::span(data_).subspan(index * columns_, columns_);
    }

    // A strided view of one column's `rows()` values (stride = columns()).
    [[nodiscard]] auto column(std::size_t index) noexcept -> ColumnView {
        return ColumnView(
            data_.data() + index,
            ColumnView::mapping_type(Extents1D(rows_), std::array<std::size_t, 1>{columns_})
        );
    }

    [[nodiscard]] auto column(std::size_t index) const noexcept -> ConstColumnView {
        return ConstColumnView(
            data_.data() + index,
            ConstColumnView::mapping_type(Extents1D(rows_), std::array<std::size_t, 1>{columns_})
        );
    }

  private:
    std::size_t rows_;
    std::size_t columns_;
    std::vector<std::uint8_t> data_;
};

} // namespace covarray
