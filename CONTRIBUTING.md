# Contributing to AuraUI

Thank you for your interest in contributing to AuraUI! We welcome contributions to make AuraUI the most secure, sleek, and high-performance cross-platform desktop framework.

---

## Development Prerequisites

### 1. Compilers & Tools
- **C++20 Compiler**:
  - Windows: Visual Studio 2022 (MSVC 19.34+) or Clang-CL
  - Linux: GCC 11+ or Clang 14+
  - macOS: Xcode 14+ (AppleClang)
- **CMake**: Version 3.24 or newer
- **Ninja**: Recommended for fast builds
- **Python**: 3.9+ for CLI and tooling
- **Node.js**: 18+ for TypeScript client package development

---

## Building and Testing

AuraUI uses `CMakePresets.json` as the single source of truth:

```bash
# Windows MSVC
cmake --preset win-msvc-release
cmake --build --preset win-msvc-release

# Linux GCC
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release

# macOS
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release
```

### Running Tests
```bash
ctest --preset unit-tests
```

### Running with Sanitizers
```bash
cmake --preset linux-clang-asan
cmake --build --preset linux-clang-asan
ctest --test-dir build/linux-clang-asan --output-on-failure
```

---

## Coding Standards

1. **Format**: Follow `.clang-format` (run `clang-format -i` before committing).
2. **Lint**: Zero warnings under `.clang-tidy`.
3. **Safety**: Never use raw pointers for resource ownership. Always use RAII wrappers (`std::unique_ptr`, `std::shared_ptr`, Win32 handle wrappers).
4. **Encoding**: Internal code MUST be UTF-8. Only platform adapters convert to native wide/OS encodings.
5. **Errors**: Core APIs must return `Result<T>` (`tl::expected<T, Error>`). Never leak unhandled exceptions across FFI or thread boundaries.
