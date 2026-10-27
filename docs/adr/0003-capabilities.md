# ADR-003: Zero-Trust Capabilities System

## Status
Accepted

## Context
Desktop web frameworks (like Electron) have suffered from critical security flaws where untrusted frontend code or vulnerable npm dependencies obtained full system access, arbitrary file access, or remote code execution.

## Decision
Adopt a **Deny-by-Default Capability Security Model**:
1. Every native capability is denied unless explicitly declared and granted in `aura.config.json`.
2. A missing, corrupted, or invalid manifest causes the application to fail closed (every capability denied).
3. The Capability Policy is immutable after application initialization.
4. Powerbox Pattern: Sensitive APIs like file access never accept raw paths from JavaScript. File dialogs return an opaque, cryptographically random 128-bit grant ID bound to the session and window. File operations accept grant IDs only.

## Consequences
- XSS or supply chain vulnerabilities in the frontend cannot escalate into system compromise.
- Developers declare explicit permission boundaries, reviewable during security audits.
