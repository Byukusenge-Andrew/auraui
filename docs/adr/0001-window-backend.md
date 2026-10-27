# ADR-001: Window Backend Architecture

## Status
Accepted

## Context
AuraUI v1 was built on top of WebUI, which launches an external Chromium/Edge browser process in "app mode" communicating over an ephemeral local HTTP/WebSocket server. This introduced significant security and UX disadvantages:
- Running an open local HTTP/WebSocket server exposes an attack surface to local processes and malicious browser extensions.
- When Microsoft Edge was the default browser, the window grouped with Edge on the taskbar and retained browser-like hotkeys (F12, F5, Ctrl+P).
- True frameless native windows could not be rendered reliably across environments without an installed Chromium-compatible browser.

## Decision
Adopt a real, in-process native webview window backend behind an internal `Backend` abstraction layer:
- **Windows**: Microsoft Edge WebView2 (in-process native control).
- **macOS**: `WKWebView` via Cocoa/AppKit.
- **Linux**: WebKitGTK (`libwebkit2gtk-4.1`).

For Phase 1 cross-platform bootstrap, use a vendored, pinned `webview/webview` library (commit `001fb173ec20dc15a0c95ef35e9f8ee4c749b5fc`) wrapped strictly behind our internal `Backend` interface.
In Phase 2, native direct backends (`webview2_backend.cpp`, `wkwebview_backend.mm`, `webkitgtk_backend.cpp`) implement custom URI schemes (`aura://app`) and custom navigation filters.

## Consequences
- **Positive**:
  - No external HTTP server or port exposed for IPC.
  - Native window handles, true frameless styling, native menus and title bars.
  - Complete control over navigation policies, web permissions, and devtools.
- **Negative**:
  - Linux requires `libwebkit2gtk-4.1` development packages at compile time and runtime.
