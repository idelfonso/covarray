# Core types

This note defines the three foundational types for Phase 1 — `Error`,
`Spec`, and `CoveringArray` — along with the storage layout and ownership
rules they share. It's the design note for the Mon Nov 23 plan day;
implementation is tracked by #31 (`Error`), #32 (`Spec`), and #33
(`CoveringArray`).

## `Error`

A plain value type returned by every fallible library operation instead of
an exception:

```cpp
enum class ErrorCode {
    invalid_strength,
    invalid_level,
    empty_columns,
    forbidden_tuple_out_of_range,
    seed_row_mismatch,
    // extended as new fallible operations need new codes
};

struct Error {
    ErrorCode code;
    std::string message;
};
```

Fallible operations return `std::expected<T, Error>`. The library never
throws from its own APIs (this becomes ADR-003, "errors as values", on Fri
Nov 27).

## `Spec`

Describes the covering array to construct:

- `strength` (`t`): the coverage strength.
- `levels`: `std::vector<std::uint8_t>`, one entry per column, each the
  number of symbols for that column (2–255). Having one level per column
  from the start means mixed-level arrays need no separate type later.
- `forbidden_tuples`: tuples of (column, value) combinations that must never
  appear together.
- `seed_rows`: optional rows the construction must extend rather than
  overwrite.

Validation (Tue Nov 24) returns `std::expected<Spec, Error>`, checking:
`t >= 2`, `t <= k` (number of columns), every level in `[2, 255]`, every
forbidden tuple references valid columns and in-range values, and every
seed row matches the column count and each column's level range.

## `CoveringArray`

The owning matrix type. It holds a single flat, owned buffer of
`std::uint8_t`, sized `N * k` for `N` rows and `k` columns.

**Symbol type is fixed to `uint8_t`, not generic.** In this project v is at
most 255 in practice (normally v ≤ 25), so a single byte per symbol always
suffices. Earlier drafts of this plan considered a `SymbolType` concept
with `uint8_t`/`uint16_t` instantiations; that genericity is dropped; the
instances this project needs to construct and verify can have very large N
and k, and halving the per-symbol footprint by committing to `uint8_t`
matters more than supporting a symbol width we'll never need.

**Storage layout is row-major by default.** `std::mdspan` views sit over
the owned buffer:

- `ca[r, c]` element access via a 2D `mdspan` with `layout_right` (row-major).
- Row views: a contiguous span of `k` elements — the natural, cache-friendly
  access pattern for most construction methods, which build or mutate one
  row at a time.
- Column views: a strided view (stride `k`) for algorithms that need a
  single column's values across all rows.

Column-major is **not** the default and is not being built now. It's
called out here only because it was an actual candidate for Phase 1's
storage layout, and it's explicitly deferred: it may be introduced later as
an alternative layout for the verification kernels specifically (Wed Dec 2,
#13), and only if a benchmark comparing row-major vs. column-major access
for that workload shows a clear, reproducible speedup. The `mdspan`-based
view API is what makes that possible without changing `CoveringArray`'s
public interface: a verification-only kernel could operate over a
differently-laid-out buffer behind the same view abstractions.

## Ownership rules

- `CoveringArray` owns its buffer and has value semantics: copyable and
  movable like `std::vector`.
- Every `std::mdspan` view is non-owning and must never outlive the
  `CoveringArray` it was taken from.
- `Spec` and `Error` are plain value types with no shared ownership.

## Open questions

- **Row-major vs. column-major verification benchmark — resolved, kept
  row-major (Tue Nov 24, #13).** Methodology: a one-off `std::chrono`
  timing comparison (not Google Benchmark — that's a later day's
  infrastructure), running the index verifier's actual kernel (one
  bitset-marking pass per t-subset of columns, over every row) against
  two storages holding the same random data — the real row-major
  `CoveringArray`, and a column-major buffer built behind the identical
  gather loop. Two fixed instances, matching `plan/GENERAL.md`'s
  performance-measurement examples (`N` picked arbitrarily at 500, since
  no construction method exists yet to produce a real minimal-`N`
  array): best-of-3, release optimizations, results stable within <1%
  across repeated runs.

  | Instance | Row-major | Column-major | Row-major is |
  | --- | --- | --- | --- |
  | CA(500; 3, 100, 3) | 310 ms | 328 ms | ~6% faster |
  | CA(500; 6, 30, 2) | 2391 ms | 1943 ms | ~19% **slower** |

  The result is reproducible but **mixed**, not a clean win either way:
  many-small-subsets workloads (low `t`, high `k`) favor row-major
  (scattered-but-nearby reads within one row beat a stride-`N` jump per
  column); few-larger-subsets workloads (higher `t`, lower `k`) favor
  column-major. That doesn't clear the bar this note already set —
  "only if a benchmark ... shows a clear, reproducible speedup" — since
  it isn't clear, only reproducible. **Decision: `CoveringArray` stays
  row-major; no column-major path is added.** Revisit only if a cost
  model (`plan/GENERAL.md`'s "a cost model fitted to benchmark data picks
  the kernel") becomes worth building for other reasons, at which point
  this split could inform which layout that model picks per instance
  shape — not as a decision to make today.
- `ErrorCode`'s full enumeration will likely grow as later days add fallible
  operations (parsing, construction); the list above covers only what
  `Spec` validation needs.
- The on-disk text format `CoveringArray` is read from and written to is a
  separate concern, specified in
  [`ca-file-format.md`](ca-file-format.md).
