# Chained Decos ⛓️

### _A High-Performance 3D Parkour Engine & Game Powered by CHEngine (Chained Engine)_

[![C++23](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/build.yml)](https://github.com/IOleg-crypto/Chained-Decos/actions)
[![raylib](https://img.shields.io/badge/raylib-5.5--dev-red?logo=raylib)](https://www.raylib.com/)

> [!NOTE] > **Active Development Notice**: This engine and game are under active development. Some information in this README may be outdated as features are continuously being added, refactored, and improved. Check the latest commits and [implementation plans](https://github.com/IOleg-crypto/Chained-Decos/tree/refactor-branch) for the most current state.

**Chained Decos** is a fast-paced, momentum-based 3D parkour game built from the ground up using **CHEngine**, a custom modular C++23 game engine. It features advanced physics, an ECS-driven architecture, native C++ scripting, and integrated development tools.

![Game Screenshot](https://i.imgur.com/d9Bxmsq.jpeg)

---

## The CHEngine Architecture

CHEngine follows a modern, modular design inspired by professional game engines:

- **Entity Component System (ECS)**: Powered by [EnTT](https://github.com/skypjack/entt) for high-performance entity management
- **Native C++ Scripting**: Custom scripting DSL for gameplay logic with hot-reloading support
- **Virtual File System**: Unified asset access with support for loose files (dev) and PAK archives (production)
- **Scene Serialization**: YAML-based scene format with full component persistence

## Editor & Simulation Workflow

The Chained Editor is the primary tool for creating and testing parkour courses.

> [!IMPORTANT] > **Cursor Control in Simulation Mode**:
> When you press **PLAY**, the editor captures the cursor for game control.
> Press **ESCAPE** again to return to simulation control.

![Editor Screenshot](https://i.postimg.cc/sXSj3qBC/Znimok-ekrana-2026-01-07-172723.png)

> [!WARNING] > **Standalone Runtime**: Standalone executable (`Runtime.exe`) is currently in development and may be unstable. It is recommended to run courses via the **ChainedEditor**.

---

### Engine Features

- **High-Performance Rendering**: Optimized Raylib-based renderer with custom shader support and PBR materials
- **Native C++ Scripting**: Type-safe gameplay scripts with DSL macros for clean, maintainable code
- **Multi-Script System**: Attach multiple scripts per entity with inter-script communication(Planning)
- **Advanced Physics**: BVH-accelerated collision detection with mesh colliders
- **Undo/Redo System**: Full command history for editor operations (Add/Delete/Move/Transform)
- **Asset Management**: Centralized loading with engine/project path resolution and texture fallback
- **Scene Serialization**: YAML-based format with UUID tracking and hierarchy preservation
- **Hot-Reloading**: Development mode supports live asset updates without restart

---

## Installation & Build

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

## Contributing & License

Contributions are welcome! This project is licensed under the **MIT License**.

Made with ❤️ using **Raylib**, **ImGui**, and **Modern C++23**.
