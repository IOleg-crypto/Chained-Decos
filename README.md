# Chained Decos ⛓️

### _A High-Performance 3D Parkour Engine & Game Powered by CHEngine_

[![C++23](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/build.yml)](https://github.com/IOleg-crypto/Chained-Decos/actions)
[![raylib](https://img.shields.io/badge/raylib-5.5--dev-red?logo=raylib)](https://www.raylib.com/)

**Chained Decos** is a fast-paced, momentum-based 3D parkour game built from the ground up using **CHEngine**, a custom modular C++23 game engine. It features advanced physics, an ECS-driven architecture, and integrated development tools.

![Game Screenshot](https://i.imgur.com/d9Bxmsq.jpeg)

---

## 🏗️ The CHEngine Architecture

CHEngine follows a modern, modular design inspired by professional game engines:

- **Modern ECS (EnTT)**: High-performance Entity Component System for efficient data management and system updates.
- **Modern Scene Management**: Integrated `ECSSceneManager` supporting seamless transitions between Main Menu, Editor, and Simulation modes.

---

## 🛠️ Editor & Simulation Workflow

The Chained Editor is the primary tool for creating and testing parkour courses.

> [!IMPORTANT] > **Cursor Control in Simulation Mode**:
> When you press **PLAY**, the editor captures the cursor for game control.
>
> - Press **ESCAPE** to unlock the cursor and interact with the UI (e.g., to click **STOP**).
> - Press **ESCAPE** again to return to simulation control.
> - **BACKSPACE** acts as an emergency simulation stop.

![Editor Screenshot](https://i.postimg.cc/sXSj3qBC/Znimok-ekrana-2026-01-07-172723.png)

> [!WARNING] > **Standalone Runtime**: Standalone executable (`Runtime.exe`) is currently in development and may be unstable. It is recommended to run courses via the **ChainedEditor**.

---

### Engine Technicals

- **High-Performance Rendering**: Optimized Raylib-based renderer with custom shader support (Skyboxes, PBR planning).
- **Undo/Redo System**: Full command history for editor operations (Object Add/Delete/Move).
- **Asset Management**: Centralized asset loading for models, textures, and scenes.

---

## 🚀 Installation & Build

### Prerequisites

- **CMake**: 3.25 or higher
- **Ninja Build System**: (Recommended for fast builds)
- **C++23 compatible compiler**: Clang 17+, GCC 13+, or MSVC 19.36+
- **Raylib Dependencies**: Standard OpenGL/X11 libraries on Linux.

### Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos

# 2. Build using CMake Presets (Choose one)

# Option A: Ninja (Recommended for fast builds)
cmake --preset ninja-release
cmake --build --preset ninja-release

# Option B: Visual Studio 2022
cmake --preset vs2022
cmake --build --preset vs2022-release

# Option C: Linux (Makefiles)
cmake --preset make-release
cmake --build --preset make-release

# 3. Run the Editor
./build/release/bin/ChainedEditor.exe # Path may vary based on preset
```

---

## 🧪 Testing & CI

We use **Google Test** for engine validation. The project includes automated CI workflows via GitHub Actions for Windows and Linux.

```bash
# Run unit and integration tests
./build/bin/tests/ChainedDecosUnitTests.exe
./build/bin/tests/ChainedDecosIntegrationTests.exe
```

---

## 🤝 Contributing & License

Contributions are welcome! This project is licensed under the **MIT License**.

Made with ❤️ using **Raylib**, **ImGui**, and **Modern C++23**.
