# ADR-007: Universal UTF-8 Internal Encoding

## Status
Accepted

## Context
Non-ASCII characters (accents, umlauts, CJK characters, emojis) often trigger buffer truncation or crashes on Windows when ANSI APIs (`GetOpenFileNameA`) or improper string conversions are used.

## Decision
1. **UTF-8 everywhere inside AuraUI**: all internal strings, JSON envelopes, configuration paths, and public APIs use standard `std::string` containing valid UTF-8.
2. Platform adapters isolate platform-specific encodings:
   - On Windows, adapters convert UTF-8 to UTF-16 wide strings immediately before invoking Win32 Wide APIs (`GetOpenFileNameW`, `SHBrowseForFolderW`, `CreateProcessW`).
3. Invalid byte sequences or surrogate pairs are validated and sanitized upon entering the C++ boundary using the Unicode replace handler, preventing crashes or parsing errors.

## Consequences
- Full Unicode support across Windows, macOS, and Linux.
- Zero crashes on non-ASCII paths, emojis, or international system user profiles.
