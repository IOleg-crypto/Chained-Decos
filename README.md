# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

Chained Decos is a fast-paced parkour game built with modern C++20 and [raylib](https://www.raylib.com/). It uses [ImGui](https://github.com/ocornut/imgui) and [rlImGui](https://github.com/raylib-extras/rlImGui) for in-game UI and debugging tools.

> [!IMPORTANT]
> This project is under active development. Expect frequent changes and engine refactors.

> [!WARNING]
> Configuration can occasionally be unstable. If you hit issues, see the Troubleshooting section.

---

## 🚀 Features
- **Parkour gameplay**: run, jump, vault, chain moves
- **Singleplayer** and *(planned)* **Multiplayer**
- **Modern C++20**, modular architecture
- **ImGui + rlImGui** for UI and debug overlays

---

## 🛠️ Quick Start

### 1) Install prerequisites (Compiler, CMake, Git)

- **Minimum versions**
  - **CMake**: 3.20+
  - **C++20 compiler**: GCC 10+, Clang 11+, or MSVC 19.28+ (Visual Studio 2019 16.8+)

#### Windows (choose one toolchain)
- Option A — MSVC (recommended for Visual Studio/CLion)
  1. Install Visual Studio (or "Build Tools for Visual Studio") with the "Desktop development with C++" workload
  2. Use the x64 toolchain ("x64 Native Tools Command Prompt for VS 2022") or let CMake/IDE detect it automatically

- Option B — MSYS2 MinGW-w64 (for CMake + Ninja/Make workflows)
  1. Install MSYS2: https://www.msys2.org/
  2. Open the "MSYS2 MinGW x64" shell and run:
     ```bash
     pacman -Syu
     pacman -S --needed mingw-w64-x86_64-toolchain git cmake ninja
     ```
  3. Add to PATH in Windows: `C:\msys64\mingw64\bin`

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake git
# Optional newer clang
sudo apt install -y clang
```

#### Fedora
```bash
sudo dnf install -y gcc-c++ cmake git
# Or clang
sudo dnf install -y clang
```

#### Arch/Manjaro
```bash
sudo pacman -S --needed base-devel cmake git
# Optional clang
sudo pacman -S clang
```

#### macOS
- Install Xcode Command Line Tools:
```bash
xcode-select --install
```
- Homebrew (recommended):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake git
# Optional newer compiler
brew install llvm
```

### 2) Clone the repository
```bash
git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos
# If you cloned without submodules
# git submodule update --init --recursive
```

### 3) Configure and build with CMake
This project uses CMake `FetchContent` to automatically fetch dependencies (raylib, ImGui, rlImGui, nlohmann_json, GoogleTest). No manual installs required beyond CMake and Git.

- Generic single-config generators (Make/Ninja):
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug   # Or Release/RelWithDebInfo/MinSizeRel
cmake --build . --config Debug
```

- Windows (Visual Studio multi-config):
```powershell
# inside build folder
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
```

- Windows (MSYS2 MinGW + Ninja):
```bash
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### 4) Run
Executables usually end up under `build/src/...` (exact path depends on generator/config).

- Windows:
```powershell
# Examples (adjust to your generator/output tree)
.\src\Game\ChainedDecosGame.exe
.\src\MapEditor\ChainedDecosMapEditor.exe  # if built
```

- Linux/macOS:
```bash
./src/Game/ChainedDecosGame
# Or the editor (if built)
./src/MapEditor/ChainedDecosMapEditor
```

> Tip: The repo may include pre-created `cmake-build-*` folders or a `clean_and_configure.bat` helper. You can use them for convenience.

---

## 🧪 Tests
```bash
# from your build directory
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest
```

---

## 🧰 Map Editor (ChainedMapEditor)
Enable the editor via `BUILD_MAP_EDITOR=ON`:
```bash
cmake .. -DBUILD_MAP_EDITOR=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```
Run (path depends on your generator):
```bash
./src/MapEditor/ChainedDecosMapEditor      # Linux/macOS
.\src\MapEditor\ChainedDecosMapEditor.exe # Windows
```

---

## 🧹 Troubleshooting
- **CMake cannot find a compiler/toolchain**
  - Windows (MSVC): open "x64 Native Tools Command Prompt for VS 2022" and reconfigure
  - Windows (MSYS2): ensure `C:\msys64\mingw64\bin` is in PATH; use `-G Ninja`
  - Linux/macOS: verify `g++ --version` or `clang++ --version`
- **Broken CMake cache / changed CMakeLists**
  - Delete `build` and reconfigure from scratch
- **Linker errors for raylib/ImGui**
  - Ensure internet connectivity on first configure (FetchContent downloads deps)
  - Clear `build` and retry
- **Wrong architecture**
  - Use consistent x64 across generator and toolchain

Clean build directory commands:
- Linux/macOS:
```bash
rm -rf build
```
- Windows PowerShell:
```powershell
Remove-Item -Recurse -Force build
```

---

## 💻 Recommended IDEs
- **Visual Studio** (Windows) — excellent CMake integration
- **CLion** — great CMake workflow
- **VS Code** — install the CMake Tools and C/C++ extensions

---

## 🎮 Controls
- **WASD**: Move
- **Space**: Jump
- **Shift**: Sprint
- **Mouse**: Look
- **F1**: Main menu
- **F2**: Debug info
- **F3**: Collision debug
- **F4**: Exit
- **F11**: Fullscreen

---

## ❓ FAQ
- **Do I need to install raylib/ImGui/rlImGui manually?**
  - No. CMake FetchContent downloads and builds dependencies automatically.
- **License?** — MIT (see LICENSE)

---

## 📚 Resources
- [raylib docs](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [ImGui wiki](https://github.com/ocornut/imgui/wiki)
- [rlImGui](https://github.com/raylib-extras/rlImGui)
- [nlohmann/json](https://github.com/nlohmann/json)

---

*Made with ❤️ using raylib, ImGui, and rlImGui*