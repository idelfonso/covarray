# Covering array text file format

This note fixes the project-wide convention for the plain-text covering
array files covarray reads and writes. The convention is settled before
any parser is built against it. It covers the text format only; CSV,
JSON, and the versioned binary format (Tue Dec 1, #12) are separate,
later formats.

## File naming

No extension is enforced and no particular name is required. A file name
may optionally embed the array's parameters (N rows, k columns, t strength,
v order) for human readability, but covarray never parses the file name —
only the file's own parameter line (below) is authoritative.

## Alphabet

For a covering array of order v, the valid symbols for an ordinary (non-
wildcard) cell are `0, 1, ..., v-1` — the elements of `Z_v`. Two additional
spellings are valid in a file:

- **`v`** itself is a valid cell value: the *wildcard* ("don't care")
  symbol — a cell that isn't used to cover any t-tuple drawn from `Z_v`.
- **`-`** (a single hyphen) is accepted as an alternate spelling of the
  wildcard, matching the NIST covering array repository's convention.

For a mixed-level array, each column has its own level `v_i` (see
"Parameter line" below), and that column's wildcard is `v_i` — there is no
single global wildcard value across a mixed-level array.

On read, `-` is normalized to the column's wildcard value (`v` or `v_i`)
immediately: covarray's in-memory `CoveringArray` only ever stores `v`/`v_i`
for a wildcard, never `-`. On write, covarray always emits the numeric
wildcard value, never `-`; `-` is accepted only for reading files from
other tools.

Any symbol outside `{0, ..., v_i - 1, v_i, '-'}` for its column is invalid.
A file containing one is rejected outright — covarray does not partially
load a file with an invalid symbol.

## Comments

Two comment styles are both supported in the same file:

- **DIMACS style**: a line whose first non-blank character is `c` or `C`,
  immediately followed by a blank, is a comment line in its entirety. (A
  word like `construct` does not start a comment: the character right
  after `c`/`C` must be blank.)
- **Python style**: `#` may appear anywhere on a line; `#` and everything
  after it to the end of the line is a comment, even when it follows real
  data on the same line (e.g. `0 1 2 # a note`).

Comment lines may appear anywhere in the file: before the parameter line,
interspersed among the N data rows, or after the last data row. They are
never counted toward N and never contribute symbols.

Multiline comments are not supported — there is no block-comment syntax.

## Parameter line

The first non-comment line gives the array's parameters as:

```
N k <level-spec> t
```

- `N`: number of rows.
- `k`: number of columns.
- `t`: strength.
- `<level-spec>`: either a single `v^k` for a uniform array (every column
  has level `v`), or a sequence `v_1^c_1 v_2^c_2 ... v_m^c_m` for a mixed-
  level array, where `c_1 + c_2 + ... + c_m = k`. The sequence assigns
  levels to columns left to right: the first `c_1` columns have level
  `v_1`, the next `c_2` columns have level `v_2`, and so on.

Examples:

- `26 3 5^3 2` is `CA(26; 2, 3, 5)`: N=26 rows, k=3 columns, every column
  level v=5, strength t=2.
- `12 6 4^1 3^2 2^3 2` is `MCA(12; 2, 4^1 3^2 2^3)`: N=12 rows, k=6 columns
  — column 1 has level 4, columns 2–3 have level 3, columns 4–6 have level
  2 — strength t=2.

## Column order for mixed-level arrays

For a mixed-level array, columns are always written in **decreasing order
of level**: column index 0 has the largest level, and levels never
increase as the column index grows. This means a written parameter line's
level-spec sequence `v_1^c_1 v_2^c_2 ... v_m^c_m` always has
`v_1 > v_2 > ... > v_m`, and each data row's values follow that same
column order.

On input, columns may appear in **any** order — the level-spec sequence in
the file being read need not be decreasing. After reading, covarray
reorders the columns (the level-spec and every data row's values together)
into decreasing-level order before the array is used or written anywhere.
Columns that share the same level keep their relative input order (a
stable sort), so re-sorting is deterministic and idempotent: reading an
already-sorted file and writing it back out reproduces the same file.

This reordering happens once, at read time; `CoveringArray` itself has no
separate "original column order" to track, and uniform arrays are
unaffected since there is only one level.

## Data rows

After the parameter line (skipping any interspersed comment lines), the
file has N data rows, each with k integers (or `-`) separated by blanks,
one row per covering array row.

## Rejecting invalid files

A file fails to load, rather than loading partially or substituting a
default, if any cell's symbol is invalid for its column, if the parameter
line is malformed, or if the number of data rows or values per row doesn't
match N and k.

## Relationship to the core types

This format is read into and written from a [`CoveringArray`](core-types.md),
using that type's fixed `uint8_t` storage. Since v is at most 255 in
practice, the wildcard value `v` (or `v_i`) always fits in the same byte
used for ordinary symbols — no separate wildcard representation is needed
in memory.
