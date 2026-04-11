# Chained Decos

## A 3D Parkour Game Built on Chained Engine

[![C++23](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/ci.yml?label=CI)](https://github.com/IOleg-crypto/Chained-Decos/actions/workflows/ci.yml)
[![SDK Deploy](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/deploy-sdk.yml?label=SDK%20Deploy)](https://github.com/IOleg-crypto/Chained-Decos/actions/workflows/deploy-sdk.yml)
[![OpenGL](https://img.shields.io/badge/graphics-OpenGL%204.3%2B-red?logo=opengl)](https://www.khronos.org/opengl/)

Chained Decos is the game project built on top of Chained Engine, a modular C++23 engine with editor tooling, runtime packaging, ECS architecture, physics, and managed gameplay scripting.

![Game Screenshot](https://i.imgur.com/MLIxRhB.png)

> [!NOTE]
> Active development is ongoing. Features and workflows continue to evolve, but this README is maintained to reflect the current repository state.

## Table of Contents

- [Overview](#overview)
- [Documentation](#documentation)
- [Editor and Simulation Workflow](#editor-and-simulation-workflow)
- [Engine Feature Highlights](#engine-feature-highlights)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build](#build)
- [Run](#run)
- [Scripting](#scripting)
- [Testing](#testing)
- [CI/CD](#cicd)
- [Troubleshooting](#troubleshooting)
- [Known Issues](#known-issues)
- [Contributing](#contributing)
- [License](#license)

## Overview

Chained Decos and Chained Engine currently target Windows and Linux.

Main capabilities:

- OpenGL 4.3+ rendering pipeline.
- ECS-driven scene model using EnTT.
- YAML-based project and scene serialization.
- Editor workflow with hierarchy/inspector/panels and in-editor play mode.
- Standalone runtime wrapper for shipping or testing project builds.
- Managed C# gameplay scripting through Coral (.NET/CoreCLR host).

## Documentation

Use these short topic pages when you want focused details without reading the whole README:

- [Architecture](docs/architecture.md)
- [Build and Run](docs/build-and-run.md)
- [Scripting](docs/scripting.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Writing Guidelines](docs/writing-guidelines.md)

## Editor and Simulation Workflow

The editor is the main authoring environment for scene creation, iteration, and play-mode testing.

> [!IMPORTANT]
> Simulation controls:
>
> - Press PLAY to enter simulation and capture cursor.
> - Press Escape to leave simulation control and return to editor interaction.

![Editor Screenshot 1](https://i.imgur.com/jey25o0.png)
![Editor Screenshot 2](https://i.imgur.com/VMhs9Zm.jpeg)
![Editor Screenshot 3](https://i.imgur.com/4RpCh2P.png)

> [!WARNING]
> ChainedRuntime is a dedicated wrapper executable for loading and running your project data without the full editor UI.

## Engine Feature Highlights

- High-performance OpenGL renderer with custom shader workflows.
- EnTT-based ECS architecture for scalable scene/entity management.
- Managed C# gameplay scripting through Coral and CoreCLR.
- BVH-assisted collision and physics systems for gameplay diagnostics.
- YAML scene/project serialization with UUID-centered identity tracking.
- Editor undo/redo command history for common content workflows.
- Asset loading pipeline with dedup-oriented task handling.
- Virtual file system support is currently planned/in-progress.

## Architecture

Chained Engine follows a layered architecture with a Hazel-inspired service/singleton baseline, adapted for this project's engine/editor/runtime split.

Core layers:

- Engine core: rendering, scene, physics, audio, assets, platform abstractions.
- Editor: content workflows, scene inspection/manipulation, panel-based tooling.
- Runtime: lightweight executable that loads and runs a project.
- Scripting bridge: C++/C# interop through Coral.Native and managed assemblies.

## Project Structure

- engine/: core engine modules (graphics, scene, physics, audio, platform, assets).
- editor/: ChainedEditor application and editor panels/tools.
- runtime/: ChainedRuntime application and runtime layer.
- scripting/: script host, glue bindings, and managed build integration.
- docs/: short topic-focused repository documentation.
- game/chaineddecos/: main game project content and gameplay scripts.
- tests/: native C++ test target (EngineTests).
- include/: third-party dependencies as git submodules.

## Dependencies

This repository relies on git submodules for core third-party libraries (for example EnTT, Assimp, Coral, ImGui, GLFW, GLM, yaml-cpp, GoogleTest, and others under include/).

Always initialize/update submodules before configuring CMake:

```bash
git submodule update --init --recursive
```

## Prerequisites

| Tool | Version | Notes |
| :--- | :--- | :--- |
| CMake | 3.31+ | Required by top-level CMake configuration. |
| Compiler | C++23 | GCC 14+, Clang 18+, or MSYS2/MinGW-w64 GCC and Clang on Windows. |
| Ninja | Latest | Recommended for fast parallel builds. |
| .NET SDK | 9.0.x | Required for managed scripting/test workflows. |
| Graphics Driver | OpenGL 4.3+ | Needed for editor/runtime rendering. |

Linux packages used by CI (Ubuntu reference):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libasound2-dev libglu1-mesa-dev \
  pkg-config libgtk-3-dev libdrm-dev libgbm-dev \
  xvfb libxkbcommon-x11-0 libgl1-mesa-dri mesa-utils
```

## Quick Start

1. Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos
git submodule update --init --recursive
```

1. Configure (examples):

```bash
# Linux (Ninja)
cmake --preset linux-clang

# Windows (MSYS2 GCC)
cmake --preset windows-gcc

# Windows (MSYS2 Clang)
cmake --preset windows-clang
```

1. Build:

```bash
# Ninja presets
cmake --build --preset linux-clang --parallel
cmake --build --preset windows-gcc --parallel
cmake --build --preset windows-clang --parallel
```

1. Run editor:

```bash
# Linux
./build/linux-clang/bin/ChainedEditor

# Windows
.\build\windows-gcc\bin\ChainedEditor.exe
```

## Build

CMake presets defined in CMakePresets.json:

- linux-gcc
- linux-clang
- windows-gcc
- windows-clang

Notes:

- BUILD_TESTS defaults to ON in presets.
- If you switch presets or major toolchains, do a clean configure for that build directory.
- Optional compiler cache support is built in through ccache/sccache integration in CI and CMake options.

## Run

Binary outputs are generated under build/{preset}/bin.

Editor:

```bash
# Linux
./build/linux-clang/bin/ChainedEditor

# Windows
.\build\windows-gcc\bin\ChainedEditor.exe
```

Runtime:

```bash
# Positional project path
./build/linux-clang/bin/ChainedRuntime path/to/project.chproject

# Or explicit flag form
./build/linux-clang/bin/ChainedRuntime --project path/to/project.chproject
./build/linux-clang/bin/ChainedRuntime --project path/to/project.chproject --name "My Runtime" --width 1600 --height 900

# Windows example
.\build\windows-gcc\bin\ChainedRuntime.exe --project path\to\project.chproject --name "My Runtime" --width 1600 --height 900
```

Runtime CLI flags currently supported:

- --project or -p
- --name
- --width
- --height

Editor play-mode note:

- Enter Play mode to capture cursor.
- Press Escape to return control to editor interaction.

## Scripting

Gameplay scripting is managed C# (not native C++ gameplay scripting), hosted through Coral/CoreCLR.

Relevant scripting parts:

- scripting/scriptengine.h and scripting/scriptengine.cpp for host lifecycle and assembly handling.
- scripting/script_glue_*.cpp for native-to-managed bindings.
- scripting/managed/CHEngine.Managed.csproj for managed engine API assembly.
- game/chaineddecos/src for gameplay-side C# scripts.

Managed artifacts are built as part of the scripting target when dotnet is available.

## Testing

There are two test layers.

Native tests (GoogleTest + CTest):

```bash
# Build native test target
cmake --build --preset windows-gcc --target EngineTests --parallel

# Windows Clang variant
cmake --build --preset windows-clang --target EngineTests --parallel

# Linux variant
cmake --build --preset linux-clang --target EngineTests --parallel

# Run native tests
ctest --test-dir build/windows-gcc --output-on-failure

# Windows Clang variant
ctest --test-dir build/windows-clang --output-on-failure

# Linux variant
ctest --test-dir build/linux-clang --output-on-failure
```

Managed gameplay tests (xUnit):

```bash
dotnet restore ./game/chaineddecos/scripts/tests/ChainedDecos.Scripts.Tests.csproj
dotnet test ./game/chaineddecos/scripts/tests/ChainedDecos.Scripts.Tests.csproj -c Release --no-restore
```

## CI/CD

CI workflow (.github/workflows/ci.yml):

- Builds Linux and Windows matrix across presets/configurations.
- Runs CTest for native tests.
- Runs managed script tests with .NET 9.0.x.
- Uses software rendering setup for Linux test execution (xvfb + Mesa environment variables).

Deploy workflow (.github/workflows/deploy-sdk.yml):

- Triggered by v* tags or manual dispatch.
- Runs managed tests.
- Builds and packages ChainedEditor and ChainedRuntime artifacts for Linux/Windows.

## Troubleshooting

Submodule errors during configure/build:

```bash
git submodule update --init --recursive
```

Generator switch conflicts:

- If reusing a build directory with another generator family, reconfigure from a clean build folder for that preset.

Managed build not available:

- Ensure dotnet SDK 9.0.x is installed and available in PATH.

Headless Linux test issues:

- Install the Linux packages listed in Prerequisites.
- Use xvfb and Mesa software rendering for CI-like environments.

## Known Issues

- Some native test areas are currently being reworked and may be skipped or gated in CI depending on environment constraints.
- Runtime and editor workflows are under active iteration.
- Virtual file system support is planned/in-progress and should not be treated as fully delivered yet.

## Contributing

Contributions are welcome.

- Open issues for bugs/regressions.
- Submit pull requests for fixes and improvements.
- Platform/build workflow improvements are especially helpful.
- Follow docs/writing-guidelines.md for the comment and documentation style used in this repository.

## License

This project is licensed under MIT. See [license](license) for details.
