# Shadow Dimension

A monochromatic atmospheric precision platformer in C++17 & Raylib inspired by the aesthetic of *Limbo* and *Inside*.

---

## 🎮 How to Play

- **[A] / [D]** — Move left / right
- **[SPACE]** — Jump / Wall jump
- **[SHIFT] or [F]** — Phase Shift (toggle between Black and White dimensions)
- **[ESC]** — Pause / Open Menu / Resume
- **[S] or [TAB]** — Skip level (in Menu)
- **[R]** — Restart area

---

## 🛠 Building the Project

### 🪟 Windows

1. Simply double-click or run:
   ```cmd
   build.bat
   ```
   *If build tools or Raylib are missing, `build.bat` will automatically download them via PowerShell and compile.*

2. Run the game:
   ```cmd
   shadow_dimension.exe
   ```

---

### 🐧 Linux (Ubuntu / Debian / Arch / Fedora)

1. Install dependencies:
   - **Ubuntu/Debian**:
     ```bash
     sudo apt update && sudo apt install -y build-essential cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
     ```
   - **Arch Linux**:
     ```bash
     sudo pacman -S base-devel cmake raylib
     ```

2. Build and run:
   ```bash
   chmod +x build.sh
   ./build.sh
   ./shadow_dimension
   ```
   *(CMake will automatically fetch Raylib 5.0 if not installed globally).*

---

### 🍏 macOS

1. (Optional) Install Raylib via Homebrew:
   ```bash
   brew install raylib cmake
   ```

2. Build and run:
   ```bash
   chmod +x build.sh
   ./build.sh
   ./shadow_dimension
   ```

---

### 🚀 Continuous Integration (GitHub Actions)

This repository includes a `.github/workflows/build.yml` workflow that automatically builds binaries for **Windows, Linux, and macOS** on every push. You can download ready-to-run executables from the GitHub Actions tab.
