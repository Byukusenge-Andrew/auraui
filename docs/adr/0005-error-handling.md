# ADR-005: Error Handling Across Boundaries

## Status
Accepted

## Context
Throwing C++ exceptions across C callback boundaries, asynchronous worker threads, or FFI webview interfaces leads to undefined behavior, `std::terminate`, or application crashes.

## Decision
1. Internal core APIs return `Result<T>` implemented via `tl::expected<T, Error>`.
2. `Error` objects carry a stable enumeration code:
   - `invalid_argument`
   - `permission_denied`
   - `not_found`
   - `rate_limited`
   - `unsupported`
   - `cancelled`
   - `internal`
3. Any unexpected standard exception thrown inside user middleware or handlers is intercepted at the `Dispatcher` boundary and converted to an `internal` error code without leaking sensitive host system details.

## Consequences
- Guaranteed no unhandled exception aborts across threads or FFI layers.
- Predictable, type-safe error handling for both C++ backend and TypeScript frontend.
