# ADR-004: Asset Serving and Origin Isolation

## Status
Accepted

## Context
Serving local HTML/CSS/JS frontend files securely into a webview without triggering CORS issues, file:// protocol vulnerabilities, or DNS rebinding requires careful origin isolation.

## Decision
- **Phase 1**: Serve embedded assets via an in-memory loopback HTTP server bound exclusively to `127.0.0.1` on port 0 (dynamic kernel-assigned port).
  - Enforce strict `Host` header validation to neutralize DNS rebinding attacks.
  - Apply strict Content Security Policy (CSP), `X-Content-Type-Options: nosniff`, and `Referrer-Policy: no-referrer`.
  - IPC is never exposed over this HTTP server; IPC exclusively uses native window bindings.
- **Phase 2**: Replace loopback server with custom scheme `aura://app` using native webview scheme handlers (`WebResourceRequested` on WebView2, `WKURLSchemeHandler` on WKWebView, `webkit_web_context_register_uri_scheme` on WebKitGTK).

## Consequences
- Eliminates file:// protocol sandbox ambiguities.
- Full CSP enforcement prevents arbitrary script execution.
