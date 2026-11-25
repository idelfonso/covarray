# Covering array binary file format

This note fixes the project-wide convention for the versioned binary
covering array format. Like
[the text format](ca-file-format.md), this is settled before the
reader/writer is built against it. Unlike the text and CSV formats, this
one is a machine-interchange format, not meant to be hand-authored or
hand-edited, so several of the text format's conventions (comments, the
`-` wildcard spelling, reordering mixed-level columns into decreasing-level
order) don't carry over — there's no human reader to serve them.

## Layout

All multi-byte integer fields are **little-endian** on disk, regardless of
the host's native endianness; a host that isn't little-endian (`<bit>`'s
`std::endian::native != std::endian::little`) byte-swaps on both read and
write. Single-byte fields (the version, levels, and cells) have no
endianness to worry about.

| Offset | Size (bytes) | Field | Type |
| --- | --- | --- | --- |
| 0 | 4 | Magic bytes, ASCII `"CVAR"` | — |
| 4 | 1 | Format version (currently `1`) | `uint8_t` |
| 5 | 8 | `N` (row count) | `uint64_t`, little-endian |
| 13 | 8 | `k` (column count) | `uint64_t`, little-endian |
| 21 | 4 | `t` (strength) | `uint32_t`, little-endian |
| 25 | `k` | Per-column levels, one byte each | `uint8_t[k]` |
| 25 + `k` | `N * k` | Cells, row-major | `uint8_t[N][k]` |

The fixed 25-byte header is read in full before anything else; a file
shorter than that, with the wrong magic bytes, or with an unsupported
version is rejected outright. After the header, the file's total length
must equal exactly `25 + k + N*k` bytes — anything else (truncated or
padded) is also rejected, rather than guessed at.

## Why `N`/`k` are 8 bytes and `t` is 4

`N` and `k` get the wider field because, per
[the core types note](core-types.md), this project explicitly targets very
large N and k; `t` (the strength) has no equivalent reason to be wide — a
covering array's strength is a small, practically-bounded number.

## No wildcard spelling, no column reordering

A wildcard cell is always written as its numeric value (`v_i`, the
column's level) — the same normalized form the text/CSV readers produce
internally — never as a separate marker byte. There's no `-`-style
alternate spelling to accept on read, unlike the text format: that
spelling exists to match a convention from hand-authored files (the NIST
covering array repository's), which doesn't apply here.

Likewise, columns are kept in whatever order the file's levels/cells give
them. The text format's decreasing-level column reordering exists to give
hand-authored and hand-read files a single canonical, deterministic
column order; a binary file has no such reader, so there's nothing to
canonicalize for.

## Rejecting invalid files

The whole file fails to load — never a partial read — on: a short read
(fewer than 25 bytes), wrong magic bytes, an unsupported version, a total
length that doesn't match `25 + k + N*k`, or a cell value greater than its
column's level. A column's level itself is validated the same way every
other reader's is: by `Spec::create`, which rejects a level below 2 — this
format doesn't duplicate that check.

## Relationship to the core types

Like the text format, this is read into and written from a
[`CoveringArray`](core-types.md) and a [`Spec`](core-types.md), using
`CoveringArray`'s fixed `uint8_t` storage — a cell byte on disk maps
directly to a stored cell, no conversion needed either way.
