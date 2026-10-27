# ADR-006: Build System and Presets

## Status
Accepted

## Context
Building cross-platform C++20 applications on Windows, Linux, and macOS requires consistent compiler flags, sanitizers, and dependency resolution. Ad-hoc Python or shell build scripts are brittle.

## Decision
1. **CMake 3.24+ with `CMakePresets.json`** is the single source of truth for all builds and CI jobs.
2. Standard presets:
   - `win-msvc-debug`, `win-msvc-release`
   - `linux-gcc-release`, `linux-clang-asan`
   - `macos-arm64-release`, `macos-universal-release`
3. The Python CLI (`auraui`) acts solely as an ergonomic developer wrapper executing CMake presets and asset bundle commands under the hood.

## Consequences
- Uniform builds across local developer machines and CI pipelines.
- Standard IDE integration (VS Code, Visual Studio, CLion) works out-of-the-box.
