
# ChainedEngine

## Custom C++/C# Game Engine with Editor, OpenGL Renderer, and Managed Scripting

[![C++23](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Workflow Status](https://github.com/IOleg-crypto/Chained-Engine/actions/workflows/ci.yml/badge.svg?branch=opengl)
[![Linux](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Engine/ci.yml?branch=opengl&job=Linux&label=Linux)](https://github.com/IOleg-crypto/Chained-Engine/actions/workflows/ci.yml)
[![Windows](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Engine/ci.yml?branch=opengl&job=Windows&label=Windows)](https://github.com/IOleg-crypto/Chained-Engine/actions/workflows/ci.yml)
[![OpenGL](https://img.shields.io/badge/graphics-OpenGL%204.3%2B-red?logo=opengl)](https://www.khronos.org/opengl/)

ChainedEngine is a modular C++23 game engine with editor tooling, runtime packaging, ECS architecture, physics, OpenGL 4.3+ rendering, and managed C# gameplay scripting via Coral. Ships with a parkour game **Chained Decos**.

![Game Screenshot](https://i.imgur.com/MLIxRhB.png)

> [!NOTE]
> Active development is ongoing. Features and workflows continue to evolve, but this README is maintained to reflect the current repository state.

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Build](#build)
- [Run](#run)
- [Working with Projects](#working-with-projects)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Prerequisites](#prerequisites)
- [Testing](#testing)
- [CI/CD](#cicd)
- [Troubleshooting](#troubleshooting)
- [Known Issues](#known-issues)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

## Overview

Chained Decos and Chained Engine target Windows and Linux.

- OpenGL 4.3+ rendering pipeline with PBR, fog, and shadow mapping
- ECS-driven scene model using EnTT
- YAML-based project and scene serialization
- Editor with hierarchy/inspector/panels and in-editor play mode
- Managed C# gameplay scripting through Coral (.NET/CoreCLR)
- Project export pipeline with compressed asset packs (ZSTD)
- Visual Animation Graph system (`.chag`)

> **Inspiration:** ChainedEngine is inspired by [Hazel](https://github.com/TheCherno/Hazel) by TheCherno, with significant custom additions — C# scripting via Coral, Jolt physics, animation graph system, project export pipeline, and multi-platform support.

![Editor Screenshot 1](https://i.imgur.com/jey25o0.png)
![Editor Screenshot 2](https://i.imgur.com/VMhs9Zm.jpeg)

## Quick Start

**Clone:**
```bash
git clone --recurse-submodules https://github.com/IOleg-crypto/ChainedEngine.git
cd ChainedEngine
git submodule update --init --recursive
```

**Configure + Build + Run:**
```bash
# Linux
cmake --preset linux-clang
cmake --build --preset linux-clang --parallel
./build/linux-clang/bin/ChainedEditor

# Windows (MSYS2 Clang)
cmake --preset windows-clang
cmake --build --preset windows-clang --parallel
.\build\windows-clang\bin\ChainedEditor.exe

# Windows (MSVC Ninja)
cmake --preset windows-msvc
cmake --build --preset windows-msvc --parallel

# Windows (VS 2026 .sln)
cmake --preset windows-vs2026
```

> **Editor play mode:** Press PLAY to enter simulation and capture cursor. Press Escape to return to editor interaction.

## Build

### Presets

| Preset | Generator | Compiler | Use case |
| :--- | :--- | :--- | :--- |
| `windows-clang` | Ninja Multi-Config | Clang (MSYS2) | Primary dev |
| `windows-msvc` | Ninja Multi-Config | MSVC (cl) | MSVC Ninja |
| `windows-vs2026` | Visual Studio 18 2026 | MSVC | VS solution / CI |
| `linux-clang` | Ninja Multi-Config | Clang | Linux CI |
| `linux-gcc` | Ninja Multi-Config | GCC | Linux |
| `windows-gcc` | Ninja Multi-Config | GCC (MinGW) | MinGW |

**Key CMake variables:**
- `CH_ACTIVE_GAME` — `chaineddecos` (default) or `testproject`. Build-time only. Switching requires reconfigure.
- `BUILD_TESTS` — ON by default.
- `CH_ENGINE_SHARED` — OFF by default (static engine).

If you use Clang on Windows and see Intellisense errors in VS Code, ensure `.vscode/settings.json` points to the correct build dir:
```json
"clangd.arguments": ["--compile-commands-dir=${workspaceFolder}/build/windows-clang"]
```

## Run

Binaries are generated under `build/{preset}/bin/`:

```bash
# Editor
./build/linux-clang/bin/ChainedEditor
.\build\windows-clang\bin\ChainedEditor.exe

# Runtime
./build/linux-clang/bin/ChainedRuntime path/to/project.chproject
.\build\windows-clang\bin\ChainedRuntime.exe --project path\to\project.chproject --name "My Runtime" --width 1600 --height 900
```

Runtime CLI: `--project` / `-p`, `--name`, `--width`, `--height`.

## Working with Projects

The engine supports multiple game projects under `game/`. Currently:
- **`chaineddecos`** — the main parkour game
- **`testproject`** — a lightweight sandbox for testing features

### Switching the Active Game

```bash
cmake -S . -B build/windows-clang -DCH_ACTIVE_GAME=testproject
```

Or in VS Code: Command Palette → `CMake: Edit CMake Cache (UI)` → change `CH_ACTIVE_GAME`.

### Creating a New Project

Use the scaffolding script:
```bash
python tools/create_game.py MyGame
python tools/create_game.py MyGame --csproj assets/scripts/MyGame.Scripts.csproj
```

This creates `game/mygame/` with:
- `CMakeLists.txt` — wired into the build via auto-discovery
- `src/main.cpp` — `CreateApplication` entry point
- `src/game_module.cpp` — component registration stub
- `MyGame.chproject` — project metadata
- `assets/scripts/` — for C# scripts

New games are auto-discovered by the root `CMakeLists.txt` — any directory under `game/` with a `CMakeLists.txt` is included when `CH_ACTIVE_GAME` matches.

### Editor‑created projects

The editor (**Project → New Project**) now also generates CMake build scaffolding:

- `{project}/CMakeLists.txt` — `chained_add_game()` boilerplate
- `{project}/src/main.cpp` — `CreateApplication` entry point

For a standalone build, move the project directory under `game/` (e.g. `game/mygame/`) so it is auto‑discovered by the root `CMakeLists.txt`.

### Project Configuration (`.chproject`)

Each game has a YAML metadata file defining its entry scene, physics, rendering, and window settings. See [User Guide](docs/USER_GUIDE.md) for the full reference.

## Project Structure

- `engine/` — core engine modules (graphics, scene, physics, audio, platform, assets)
- `editor/` — ChainedEditor application and editor panels/tools
- `runtime/` — ChainedRuntime application and runtime layer
- `engine/scripting/` — script host, glue bindings, and managed build integration
- `game/chaineddecos/` — main game project
- `game/testproject/` — alternate sandbox project
- `tests/` — native C++ tests (GoogleTest)
- `thirdparty/` — third-party dependencies (git submodules)
- `tools/` — build scripts, glue code generator, resource sync

## Dependencies

| Library | Purpose |
| :--- | :--- |
| EnTT | ECS framework |
| Assimp | 3D model import |
| Coral | C#/C++ interop |
| ImGui + ImGuizmo | Editor UI |
| GLFW + GLAD | Window/OpenGL |
| GLM | Math library |
| yaml-cpp | YAML serialization |
| GoogleTest | Unit/integration tests |
| JoltPhysics | Physics simulation |
| zstd + pack (cfnptr) | Asset pack compression |
| miniaudio | Audio |
| spdlog | Logging |
| stb | Image loading |
| cereal | Binary serialization |
| reflect-cpp | Runtime reflection |

Always init submodules before building:
```bash
git submodule update --init --recursive
```

## Prerequisites

| Tool | Version | Notes |
| :--- | :--- | :--- |
| CMake | 3.31+ | Required by top-level CMakeLists |
| Compiler | C++23 | Clang 18+, MSVC (VS2022+), or MSYS2/MinGW-w64 |
| Ninja | Latest | Recommended for fast parallel builds |
| .NET SDK | 10.0.x | Required for managed scripting |
| Graphics Driver | OpenGL 4.3+ | Needed for rendering |

Linux packages (Ubuntu reference):
```bash
sudo apt-get install -y build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libasound2-dev libglu1-mesa-dev \
  pkg-config libgtk-3-dev libdrm-dev libgbm-dev \
  xvfb libxkbcommon-x11-0 libgl1-mesa-dri mesa-utils
```

## Testing

Native tests use GoogleTest + CTest, split into unit (fast, no engine runtime) and integration (full engine) targets.

```bash
# Run all tests
ctest --test-dir build/windows-clang --output-on-failure

# Unit tests only
ctest --test-dir build/windows-clang -L Unit --output-on-failure

# Integration tests only
ctest --test-dir build/windows-clang -L Integration --output-on-failure
```

Managed (C#) tests require .NET SDK 10.0.x on PATH. Without it, the scripting target is silently skipped.

## CI/CD

CI workflow (`.github/workflows/ci.yml`) fans out to:
- **Format Check** (`format.yml`): `clang-format-18 --dry-run -Werror` on changed C++ files
- **Linux Builds** (`linux.yml`): Debug + Release under `xvfb` + Mesa software rendering
- **Windows Builds** (`windows.yml`): Debug + Release matrix (Clang, MSVC, GCC)

Debug builds use `-DENABLE_SANITIZERS=ON` (ASan + UBSan). CTest output is captured as JUnit XML.

Deploy workflow (`.github/workflows/deploy-sdk.yml`): triggered by `v*` tags, packages ChainedEditor + ChainedRuntime artifacts.

## Troubleshooting

- **Submodule errors:** `git submodule update --init --recursive`
- **Generator conflicts:** Reconfigure from a clean build folder when switching generator families
- **No managed build:** Ensure `dotnet` SDK 10.0.x is on PATH
- **Linux headless:** Install packages from Prerequisites, use `xvfb` + Mesa
- **Stale files after `CH_ACTIVE_GAME` change:** Reconfigure the build directory, don't just rebuild

## Known Issues

- Font system needs rework for in-scene text and editor
- Some native tests are being reworked and may be skipped in CI
- Runtime and editor workflows are under active iteration
- Virtual file system is planned/in-progress
- **Runtime may have bugs and issues — known problems include font rendering, physics edge cases, and occasional crashes under specific scenarios**

## Documentation

Full guides and references:

| Document | Description |
| :--- | :--- |
| [User Guide](docs/USER_GUIDE.md) | Step-by-step: build, run, create scenes, write scripts, export |
| [Engine Architecture](docs/ARCHITECTURE.md) | Bootstrapping, system initialization, main loop |
| [Component Reference](docs/COMPONENTS.md) | All ECS components, adding new ones |
| [Scripting API](docs/SCRIPTING_API.md) | C# API reference: lifecycle, entities, input, UI |
| [Scripting Interop](docs/SCRIPTING_INTEROP.md) | C++/C# bridge internals |
| [Animation Graphs](docs/ANIMATION_GRAPHS.md) | Visual animation graph system tutorial |
| [Export Guide](docs/EXPORT.md) | Project packaging and distribution |
| [FAQ](docs/FAQ.md) | Common patterns and solutions |

## Contributing

- Open issues for bugs/regressions
- Submit pull requests for fixes and improvements
- Platform/build workflow improvements are especially helpful

## License

This project is licensed under MIT. See [license](license) for details.
