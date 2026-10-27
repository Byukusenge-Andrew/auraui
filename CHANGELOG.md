# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.0.0] - 2026-10-27

### Added
- **True In-Process Native Webview Backend**: Dropped WebUI browser-launcher model in favor of native in-process webview controls (WebView2 on Windows, WKWebView on macOS, WebKitGTK on Linux).
- **Zero-Trust Capabilities System**: Deny-by-default capability engine governed by `aura.config.json` manifest.
- **Powerbox Pattern for File Access**: Dialogs return opaque, cryptographically random grant tokens instead of raw file paths.
- **Unified IPC Envelope (`__aura_ipc`)**: Standardized versioned message envelope with centralized Dispatcher choke point, eliminating arbitrary window method leaks.
- **Cross-Platform CMake Architecture**: CMake 3.24+ with `CMakePresets.json` as the single source of truth for MSVC, GCC, and Clang builds.
- **Hardened Security Profiles**: ASLR, DEP, Control Flow Guard (`/guard:cf`), stack protection, CET, and strict Content Security Policies.
- **Comprehensive Test Suite**: Doctest unit tests, libFuzzer fuzzing targets, and headless platform contract tests.

### Changed
- Refactored C++ header architecture into modular, focused subsystems: `unicode`, `system`, `storage`, `router`, `watcher`, `app`.
- Static CRT linkage (`/MT`) on Windows so executables run standalone without requiring Microsoft Visual C++ Redistributable DLLs.
- Unicode UTF-8 end-to-end string processing with zero crashes on non-ASCII or international paths.

### Removed
- Removed external WebUI dependency and local WebSocket/HTTP IPC server.
- Removed legacy `auraui.cmd` script and prebuilt `webui-2-static.lib` binary.
