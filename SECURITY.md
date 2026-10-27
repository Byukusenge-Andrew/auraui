# Security Policy

AuraUI takes security seriously. As a desktop application framework that bridges web frontends with native operating system capabilities, our primary security model is **Zero Trust towards the frontend**: even in the presence of malicious scripts or compromised npm dependencies, the underlying operating system and user files must remain protected.

---

## Supported Versions

Only the latest major version receives security updates.

| Version | Supported          |
| ------- | ------------------ |
| 2.x.x   | :white_check_mark: |
| 1.x.x   | :x: (Deprecated)   |
| < 1.0   | :x:                |

---

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues or discussions.**

Instead, report vulnerabilities privately through GitHub Security Advisories:
1. Navigate to the [Security Tab](https://github.com/Byukusenge-Andrew/auraui/security) on GitHub.
2. Click **"Report a vulnerability"** to open a private advisory draft.
3. Provide:
   - A detailed description of the issue.
   - Proof-of-concept code, reproducer project, or steps to reproduce.
   - The affected operating system(s) (Windows, Linux, macOS) and architecture.
   - Assessment of impact (e.g., sandbox escape, remote code execution, capability bypass).

If you are unable to use GitHub Security Advisories, email the security team directly at:
**`security@auraui.dev`** (or contact the repository maintainer directly).

### Target Response Times
- **Initial acknowledgment**: Within 48 hours.
- **Triage and severity assessment**: Within 5 business days.
- **Fix timeline and CVE coordination**: Coordinated disclosure within 90 days of confirmation.

---

## Safe Harbor Statement

We consider security research conducted in good faith to be authorized. If you:
- Make a good faith effort to avoid privacy violations, destruction of data, and interruption or degradation of services,
- Give us reasonable time to remedy the vulnerability before disclosing it publicly, and
- Do not exploit a vulnerability beyond what is strictly necessary to demonstrate it,

we will not initiate legal action against you or request law enforcement investigation regarding your research.
