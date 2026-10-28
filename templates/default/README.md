# AuraUI Application

This template provides a minimal, ultra-fast setup to build native desktop applications using **AuraUI 2.0** with **C++20** and standard **HTML5 / CSS3 / JavaScript**.

---

## 🚀 Quick Start

Run the following commands inside your project root:

### 1. Development Mode (Live Reload)
```bash
aura dev
```
> Or run `dev.bat` on Windows.
>
> Starts your desktop app with hot-reloading for UI assets. Edit anything in `ui/` (`index.html`, `style.css`, `app.js`) and changes will reflect immediately without recompiling.

### 2. Production Build (Single Standalone Executable)
```bash
aura build
```
> Or run `build.bat` on Windows.
>
> Bundles all HTML, CSS, JavaScript, and assets into virtual memory (`vfs.h`) and compiles an optimized, standalone binary in `dist/`. No Node.js runtime, no Chromium installer, no external dependencies.

---

## 📁 Project Anatomy

```text
my-aura-app/
├── aura.config.json     # Window settings, dimensions & capability permissions
├── .gitignore           # Preconfigured for MSVC, CMake, and build outputs
├── dev.bat              # Quick double-click script for development
├── build.bat            # Quick double-click script for release compilation
├── README.md            # Project starter documentation
├── src/
│   └── main.cpp         # C++20 engine entry point & native RPC bindings
└── ui/
    ├── aura.css         # AuraUI desktop glassmorphism design system
    ├── aura.js          # Client-side C++ RPC & native OS bridge
    ├── index.html       # Application HTML5 structure
    ├── style.css        # Application custom styles
    └── app.js           # Frontend logic & event handlers
```

---

## ⚡ How It Works: C++ and Frontend Bridge

### 1. Define C++ Logic
In `src/main.cpp`, expose native functions to the frontend with `app.bind`:

```cpp
app.bind("calculate", [](const auraui::Context& ctx) {
    double a = ctx.get<double>("a", 0.0);
    double b = ctx.get<double>("b", 0.0);
    std::string op = ctx.get<std::string>("operation", "+");

    double result = (op == "+") ? (a + b) : (a - b);
    return auraui::json{
        {"result", result}
    };
});
```

### 2. Call from JavaScript
In `ui/app.js`, invoke your native C++ code seamlessly with Promises:

```javascript
const response = await aura.call("calculate", {
    a: 15,
    b: 27,
    operation: "+"
});
console.log(response.result); // 42
```

---

## 🛡️ Built-in Native OS Modules

AuraUI provides secure, high-level APIs directly in `aura.js`:

| Module | Method | Description |
| :--- | :--- | :--- |
| **Window** | `aura.window.minimize()` | Minimize window |
| | `aura.window.maximize()` | Maximize / restore window |
| | `aura.window.close()` | Close application window |
| **Dialogs** | `aura.dialog.open({ title, filter })` | Native file picker dialog |
| | `aura.dialog.save({ title, defaultName })` | Native save file dialog |
| | `aura.dialog.folder()` | Native folder selection dialog |
| **Clipboard** | `aura.clipboard.writeText(text)` | Copy text to system clipboard |
| | `aura.clipboard.readText()` | Read text from system clipboard |
| **Shell** | `aura.shell.openExternal(url)` | Open URL in user's default browser |

Permissions for these modules are governed by `aura.config.json`.

---

## ⚙️ Window & Security Configuration

Edit `aura.config.json` to customize window presentation and security limits:

```json
{
  "window": {
    "title": "My AuraUI Application",
    "width": 1050,
    "height": 720,
    "min_width": 800,
    "min_height": 500,
    "frameless": false,
    "resizable": true
  },
  "capabilities": {
    "dialog.open": true,
    "clipboard.write": true,
    "shell.open_external": true
  }
}
```

---

## 📖 Official Resources

- [AuraUI Documentation](https://byukusenge-andrew.github.io/auraui/)
- [GitHub Repository](https://github.com/Byukusenge-Andrew/auraui)
- [Issue Tracker & Discussions](https://github.com/Byukusenge-Andrew/auraui/issues)
