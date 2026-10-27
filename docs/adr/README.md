# 📐 Architecture Decision Records (ADRs)

This directory documents the core architectural decisions that govern the design, security, and cross-platform behavior of **AuraUI 2**.

---

## Index of Decisions

| ADR | Title | Status | Summary |
| :--- | :--- | :--- | :--- |
| **[ADR-001](0001-window-backend.md)** | **Window Backend Architecture** | `Accepted` | Adopts real in-process native webviews (WebView2 on Windows, WKWebView on macOS, WebKitGTK on Linux) behind a unified `Backend` abstraction, eliminating external browser processes and exposed local HTTP/WebSocket ports. |
| **[ADR-002](0002-ipc-envelope.md)** | **Unified IPC Envelope and Dispatcher** | `Accepted` | Mandates a single in-process bridge `window.__aura_ipc(envelope)` with strict JSON schema, audit logging, rate limiting, and 1 MiB payload caps. |
| **[ADR-003](0003-capabilities.md)** | **Zero-Trust Capabilities System** | `Accepted` | Implements a deny-by-default permission model declared via `aura.config.json`. Enforces the Powerbox pattern for file dialogs with cryptographically opaque grant IDs. |
| **[ADR-004](0004-asset-origin.md)** | **Asset Serving and Origin Isolation** | `Accepted` | Defines embedded asset serving via in-memory loopback (Phase 1) transitioning to native custom schemes `aura://app` (Phase 2), enforcing strict CSP and host validation. |
| **[ADR-005](0005-error-handling.md)** | **Error Handling Across Boundaries** | `Accepted` | Uses `tl::expected<T, Error>` across all internal APIs and intercepts exceptions at the `Dispatcher` boundary to return stable JSON error codes (`permission_denied`, `invalid_argument`, etc.). |
| **[ADR-006](0006-build-system.md)** | **Build System and Presets** | `Accepted` | Establishes CMake 3.24+ and `CMakePresets.json` as the single source of truth across platforms and CI, with the Python CLI acting as a high-level developer convenience wrapper. |
| **[ADR-007](0007-text-encoding.md)** | **Universal UTF-8 Internal Encoding** | `Accepted` | Mandates UTF-8 for all internal C++ strings, JSON payloads, and configuration files, with platform adapters handling UTF-16 wide string conversions on Windows. |

---

## ADR Template & Lifecycle

All new architectural decisions follow the format:
- **Title**: Descriptive name prefixed with sequence number (e.g. `0008-custom-protocols.md`)
- **Status**: `Proposed` | `Accepted` | `Superseded` | `Deprecated`
- **Context**: The motivation, problem statement, and alternatives considered.
- **Decision**: The chosen technical approach and design constraints.
- **Consequences**: Both positive and negative trade-offs resulting from the decision.
