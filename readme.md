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
- [Editor and Simulation Workflow](#editor-and-simulation-workflow)
- [Engine Feature Highlights](#engine-feature-highlights)
- [Architecture](#architecture)
- [Working with Projects](#working-with-projects)
- [Project Structure](#project-structure)
- [Dependencies](#dependencies)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build](#build)
- [Run](#run)
- [Gameplay Scripting (C#)](#gameplay-scripting-c)
- [Assets and Resources](#assets-and-resources)
- [Physics and Collisions](#physics-and-collisions)
- [In-Game UI](#in-game-ui)
- [Extending the Engine (C++)](#extending-the-engine-c)
- [Debugging and Profiling](#debugging-and-profiling)
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

The game selection itself is build-time, not runtime: `CH_ACTIVE_GAME` chooses which game folder is added to the build graph, while the `.chproject` file decides what the runtime opens.

## Working with Projects

The engine supports multiple game projects inside the same repository. Right now, there are two:

- **`chaineddecos`**: the main parkour game.
- **`testproject`**: a lightweight sandbox project to test features safely.

### Switching the Active Game

To keep compile times fast, only one game project is generated in the build graph at a time. This is controlled by the `CH_ACTIVE_GAME` CMake variable.

**How to switch:**
1. **Command Line:** Run CMake with `-DCH_ACTIVE_GAME=...`.
   ```bash
   cmake -S . -B build/windows-ninja -DCH_ACTIVE_GAME=testproject
   ```
2. **VS Code:** Open the Command Palette (`Ctrl+Shift+P`), choose `CMake: Edit CMake Cache (UI)`, find `CH_ACTIVE_GAME`, change it, and save.

After changing the game, just rebuild the project. The executable name and assets will automatically switch to the new game.

### Creating a New Project

Want to start a new game from scratch? Here is how to hook it up:

1. **Create the folder structure:** Make a new directory `game/mygame`.
2. **Add a CMake script:** Create `game/mygame/CMakeLists.txt` and use the engine's helper macro:
   ```cmake
   chained_add_game(MyGameTarget
       PROJECT_GAME mygame
       CSHARP_PROJECT "scripts/MyGame.Scripts.csproj" # Omit if you don't use C# yet
   )
   ```
3. **Add the entry point:** Create `game/mygame/src/main.cpp`:
   ```cpp
   #include "engine/core/application.h"
   #include "engine/core/entry_point.h"
   #include "runtime/runtime_layer.h"

   namespace CHEngine {
       Application* CreateApplication(ApplicationCommandLineArgs args) {
           ApplicationSpecification spec;
           spec.Name = "MyGame";
           spec.CommandLineArgs = args;
           
           Application* app = new Application(spec);
           app->PushLayer(new RuntimeLayer(""));
           return app;
       }
   }
   ```
4. **Register it:** Open the *root* `CMakeLists.txt`, find the `if(CH_ACTIVE_GAME STREQUAL "...")` block, and add your new game to the list.

## Project Structure

- engine/: core engine modules (graphics, scene, physics, audio, platform, assets).
- editor/: ChainedEditor application and editor panels/tools.
- runtime/: ChainedRuntime application and runtime layer.
- scripting/: script host, glue bindings, and managed build integration.
- game/chaineddecos/: main game project, gameplay scripts under src/, and managed scripts/tests under scripts/.
- game/testproject/: alternate standalone game project used for project switching and smaller experiments.
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
- `CH_ACTIVE_GAME` defaults to `chaineddecos` in CMakePresets.json. Set it to `testproject` when you want the alternate game.
- Optional compiler cache support is built in through ccache/sccache integration in CI and CMake options.
- If you use Clang on Windows and see Intellisense errors in VS Code, ensure that `.vscode/settings.json` points to the correct build directory for `clangd`:
  ```json
  "clangd.arguments": [
      "--compile-commands-dir=${workspaceFolder}/build/windows-ninja"
  ]
  ```

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

When you build the alternate game project, the executable name changes with the active target. The runtime still discovers the `.chproject` file from the executable/project directory.

Editor play-mode note:

- Enter Play mode to capture cursor.
- Press Escape to return control to editor interaction.

## Gameplay Scripting (C#)

Chained Engine uses managed C# for gameplay, powered by Coral (.NET/CoreCLR). This means you write your game logic in C# while the heavy lifting (rendering, physics) stays in C++.

### Writing Your First Script

Scripts live inside your game's source folder (e.g., [game/chaineddecos/src](game/chaineddecos/src)). Here is a practical example of a basic script:

```csharp
using System;
using CHEngine;

namespace ChainedDecos
{
    public class PlayerController : Script
    {
        public float Speed = 5.0f;
        private TransformComponent _transform;

        protected override void OnCreate()
        {
            // Called once when the Entity is initialized
            _transform = GetComponent<TransformComponent>();
            Log.Info("Player Controller initialized!");
        }

        protected override void OnUpdate(float deltaTime)
        {
            // Called every frame
            if (Input.IsKeyDown(KeyCode.W))
            {
                Vector3 pos = _transform.Translation;
                pos.Z -= Speed * deltaTime;
                _transform.Translation = pos;
            }
        }
    }
}
```

### Connecting the Script in the Editor

Once you've written your magical gameplay code, how does the engine know about it?

1. **Build the scripts:** Either rebuild the project through CMake/Ninja, or navigate to your `.csproj` folder and run `dotnet build`.
2. **Open the Editor** and select the Entity you want to control.
3. **Add Component:** In the Inspector panel, click **Add Component** and choose **Managed Script Component**.
4. **Link it:** In the `Class Name` text field, type the **fully qualified name** of your script — namespace included (for example: `ChainedDecos.PlayerController`).
5. **Play:** Hit the Play button in the editor. The engine will instantly instantiate your C# class and execute your lifecycle methods!

### Under the Hood: Architecture & Registration

If you are modifying the engine itself, you will find the native-to-managed bridge here:
- **Native Host:** [scripting/scriptengine.h](scripting/scriptengine.h) initializes Coral and loads assemblies.
- **Interops:** Native C++ calls are exposed to C# via `script_glue.cpp`.
- **Discovery:** At startup, `ScriptTypeRegistry::Discover()` scans the game DLL for classes deriving from `CHEngine.Script`.
- **Lifecycle:** `SceneScripting` instantiates your script in C++, calls `__Init()` to cache delegates, and smoothly passes events from the C++ Scene to C#.

### Managed API Surface

- [scripting/managed/src/Script.cs](scripting/managed/src/Script.cs) defines the script lifecycle base class.
- [scripting/managed/src/Entity.cs](scripting/managed/src/Entity.cs) exposes entity/component access.
- [scripting/managed/src/SceneAndApplication.cs](scripting/managed/src/SceneAndApplication.cs) exposes scene, audio, application, time, and window helpers.
- [scripting/managed/src/Input.cs](scripting/managed/src/Input.cs) wraps input queries.
- [scripting/managed/src/Log.cs](scripting/managed/src/Log.cs) wraps logging.
- [scripting/managed/src/Math.cs](scripting/managed/src/Math.cs) provides vector and scalar helpers.
- [scripting/managed/src/UI.cs](scripting/managed/src/UI.cs) exposes minimal UI helpers.

Managed artifacts are built as part of the scripting target when dotnet is available.

## Assets and Resources

All of your 3D models, textures, animations, and sound files must go into your game's `assets/` or `resources/` folder. The engine uses a unified Asset Manager to register files and assign a UUID to them so they are not loaded multiple times.

1. **Importing:** Drag and drop your source file (e.g., `.gltf` model, `.png` texture) directly into the **Content Browser Panel** in the Editor. The system registers it.
2. **Usage:** Select the Entity in your scene, find the relevant Component (like `MeshComponent`), and assign the newly loaded asset from the browser.

## Physics and Collisions

Chained Engine uses a built-in 3D physics simulation, which is heavily used by parkour traversal scripts in `chaineddecos`.

To add physical behavior to an Entity in the Editor:
1. Click **Add Component** and select **RigidBodyComponent**. This determines if the object falls (Dynamic) or stays still (Kinematic/Static).
2. Add a physical shape like a **BoxColliderComponent** or **SphereColliderComponent**.

If you are writing a C# script (inherited from `CHEngine.Script`), you can hook into these collisions directly:

```csharp
protected override void OnCollisionEnter(Entity other)
{
    Log.Info($"Hit something: {other.Name}");
    if (other.HasComponent<DamageZone>())
    {
        // Example: Handle player taking damage
    }
}
```

## In-Game UI

While the Editor UI is drawn using ImGui, the gameplay (In-Game) UI meant for players is accessed through the managed scripting wrapper.

To draw simple HUDs or text menus:
1. Override the `OnGUI` method in your C# script.
2. Call static helpers from the `UI` class.

```csharp
protected override void OnGUI()
{
    // Draw some simple text on screen
    UI.DrawText("Stamina: 100", new Vector2(10.0f, 10.0f), Color.White);

    // Render a button and check if clicked
    if (UI.DrawButton("Restart Parkour", new Vector2(100.0f, 200.0f)))
    {
        // Restart logic here
    }
}
```

## Extending the Engine (C++)

Need performance that scripting can't provide, or want to create a brand new foundational Component? Here is the flow for a native ECS update:

1. **Define the Data:** Add a fast `struct` in `engine/scene/components.h`. We use `EnTT`, so components are simple structs. 
   ```cpp
   struct ParkourStateComponent {
       float Stamina = 100.0f;
       bool  IsWallRunning = false;
   };
   ```
2. **Support Serialization:** If you want editors to save or load it with the level, update `scene_serializer.cpp` or `yaml_extensions` so YAML knows how to read/write it.
3. **Expose It to the Editor:** Open `editor/editor_panels.cpp` and add the `ImGui` draw logic for `ParkourStateComponent` so designers can tweak its values in the Inspector panel.

## Debugging and Profiling

If `Chained Decos` ever suffers a frame-rate drop (a lag spike), do not optimize blindly. Use the tools:

- **Built-in Editor Profiler:** Open the **Profiler** panel. It displays a breakdown (in `ms`) of where your frame time went — `Rendering`, `Physics Update`, or `Scripting Update`. Check this first.
- **C# Debugging with CoreCLR:** Because the engine wraps .NET via Coral, you can attach a C# IDE debugger (like Visual Studio or Rider) to the running Engine/Editor process. Your breakpoints inside `OnUpdate` or `OnCreate` will pause the simulation.

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

- Dispatches native builds to `.github/workflows/linux.yml` and `.github/workflows/windows.yml`.
- Dispatches managed tests to `.github/workflows/managed.yml`.
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

Changing `CH_ACTIVE_GAME` without reconfiguring the build tree can leave stale generated files behind. Reconfigure the same build directory when you switch between `chaineddecos` and `testproject`.

## Known Issues

- Some native test areas are currently being reworked and may be skipped or gated in CI depending on environment constraints.
- Runtime and editor workflows are under active iteration.
- Virtual file system support is planned/in-progress and should not be treated as fully delivered yet.

## Contributing

Contributions are welcome.

- Open issues for bugs/regressions.
- Submit pull requests for fixes and improvements.
- Platform/build workflow improvements are especially helpful.
- Keep documentation changes in this README or in nearby code comments when the detail is local.

## License

This project is licensed under MIT. See [license](license) for details.
