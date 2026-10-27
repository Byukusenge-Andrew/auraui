<div align="center">
  <img src="assets/logo.svg" alt="AuraUI Logo" width="88" height="88" />
  <h1>AuraUI 2</h1>
  <p><strong>The Secure, Cross-Platform C++20 Desktop Application Framework with Web UI</strong></p>

  <a href="https://byukusenge-andrew.github.io/auraui/"><img src="https://img.shields.io/badge/Docs-GitHub%20Pages-27272a.svg" alt="Documentation" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-27272a.svg" alt="License: MIT" /></a>
  <a href="SECURITY.md"><img src="https://img.shields.io/badge/Security-Zero%20Trust-27272a.svg" alt="Security: Zero Trust" /></a>

  <p>
    📖 <a href="docs/USER_GUIDE.md"><strong>User Guide</strong></a> | 
    🛡️ <a href="SECURITY.md"><strong>Security Policy</strong></a> | 
    📐 <a href="docs/adr/0001-window-backend.md"><strong>Architecture ADRs</strong></a>
  </p>
</div>

AuraUI bridges **high-performance C++20 backend logic** with the **fluid design capabilities of modern HTML/CSS/JS**. Built with a **Zero-Trust capability model**, in-process native webview controls, and zero exposed network ports, AuraUI ensures your application remains lightweight, responsive, and secure.

---

## 💻 Operating System Requirements

AuraUI compiles down to a native desktop executable on each target operating system. Here are the exact prerequisites:

| Operating System | Backend Window Technology | Runtime Dependencies | Binary Size |
| ---------------- | ------------------------- | -------------------- | ----------- |
| **Windows 10 / 11** | Microsoft Edge WebView2 (in-process) | WebView2 Runtime *(pre-installed on Windows 10/11)*.<br>Static CRT (`/MT`) eliminates VC++ Redistributable DLL requirements. | ~750 KB – 1.2 MB |
| **macOS 12+** (Intel & Apple Silicon) | `WKWebView` (Cocoa / AppKit) | None *(system native framework)*. | ~800 KB – 1.5 MB |
| **Linux** (X11 & Wayland) | WebKitGTK (`libwebkit2gtk-4.1`) | `libwebkit2gtk-4.1-0` shared library. | ~900 KB – 1.8 MB |

---

## 🔒 20-Line Secure Example

```cpp
#include <auraui.hpp>

int main() {
    // 1. Initialize application with manifest validation & capability guards
    auraui::App app("Secure Counter App", 800, 600);

    int counter = 0;

    // 2. Register schema-validated command with audit logging
    app.use(auraui::middleware::logger());
    
    app.route("api/increment", [&counter](auraui::Req&, auraui::Res& res) {
        counter++;
        res.ok(auraui::json{{"count", counter}});
    });

    // 3. Start native in-process window with embedded UI assets
    app.listen("index.html");
    return 0;
}
```

---

## ⚡ Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/Byukusenge-Andrew/auraui.git
cd auraui

# 2. Configure with CMake Preset (Windows example)
cmake --preset win-msvc-release

# 3. Build standalone binary
cmake --build --preset win-msvc-release

# 4. Run tests
ctest --preset unit-tests
```

---

## 🛡️ Key Architectural Pillars

- **Zero-Trust Capabilities**: Deny-by-default. Every native capability (`dialog.open`, `clipboard.read`, `shell.open_url`, `fs.read_granted`) requires an explicit grant in `aura.config.json`.
- **In-Process IPC (`__aura_ipc`)**: All frontend-to-backend communication uses a single, strictly bounded envelope. IPC is never reachable over a network socket or localhost port.
- **Powerbox Grants**: File dialogs return opaque, cryptographically random grant tokens instead of raw filesystem paths.
- **Hardened Executables**: Built with Control Flow Guard (`/guard:cf`), ASLR, DEP, stack-smashing protection, and strict Content Security Policies.

---

## 📄 License

Distributed under the [MIT License](LICENSE). Third-party dependencies and their pinned licenses are documented in [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
