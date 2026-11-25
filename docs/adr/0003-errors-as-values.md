# ADR-003: Errors as values

## Status

Accepted

## Context

`Spec::create` and the new text format reader (`read_text_format`) both have
many ways to fail on untrusted input: an invalid strength, a malformed
parameter line, a symbol out of range for its column. The core library also
has to stay usable from a C API and from Python bindings later, where C++
exceptions either can't cross the boundary at all (C) or add overhead and
ecosystem friction that a typed result doesn't (Python via nanobind still
has to translate whatever crosses the boundary into its own exception
hierarchy either way). A decision was needed before more fallible functions
landed and each one picked its own convention.

## Decision

No exception crosses a public covarray API. Every fallible function returns
`std::expected<T, Error>` (or `std::expected<void, Error>` when there's no
value to return), never throws for an input-dependent failure, and never
returns a default-constructed or partially valid `T` on failure.

`Error` (`include/covarray/error.hpp`) is a plain `{ErrorCode code; std::string
message;}`. `ErrorCode` is a closed `enum class`: every new failure mode adds
a new enumerator rather than reusing an existing one with a different
`message`, so callers (and tests) can switch on `code` instead of parsing
`message`. `message` exists for humans — logs, CLI output, test failure
text — and is never meant to be parsed.

This only covers *input-dependent* failures — a bad `Spec`, a malformed
file, an out-of-range symbol. A precondition violation (calling a function
with an argument the type system could have ruled out, a logic bug) is still
a bug, not a value to hand back to the caller, and stays an assertion or
`std::unreachable()`, not an `Error`.

Composing fallible calls uses early return on `!result`
(`if (auto r = f(); !r) { return std::unexpected(r.error()); }`) throughout
today's code rather than `std::expected`'s monadic `and_then`/`transform`/
`or_else`; nothing currently chains enough fallible steps to make the
monadic form clearly more readable, but it's the planned style once
`solve()` and the construction pipeline exist.

## Consequences

- A C API (`covarray_c.h`, Phase 9) maps cleanly onto this: `Error` becomes
  an error code plus a message buffer the caller owns, with no exception
  machinery to bridge.
- Every fallible call site is visibly fallible at the type level — callers
  can't accidentally ignore a failure the way an uncaught exception six
  frames up could.
- A stack trace in debug builds (`std::stacktrace`) is planned for
  `Error`, which doesn't carry one yet. That's deferred until
  a call site exists where it earns its cost, not designed in speculatively
  now.
- `std::bad_alloc` and friends are explicitly out of scope: this ADR is
  about *this library's* input-dependent failures, not about being
  exception-free in the face of resource exhaustion.
