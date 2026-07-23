
# ChainedEngine

## Custom C++/C# Game Engine with Editor, OpenGL Renderer, and Managed Scripting

[![C++23](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/ChainedEngine/ci.yml?label=CI)](https://github.com/IOleg-crypto/ChainedEngine/actions/workflows/ci.yml)
[![SDK Deploy](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/ChainedEngine/deploy-sdk.yml?label=SDK%20Deploy)](https://github.com/IOleg-crypto/ChainedEngine/actions/workflows/deploy-sdk.yml)
[![OpenGL](https://img.shields.io/badge/graphics-OpenGL%204.3%2B-red?logo=opengl)](https://www.khronos.org/opengl/)

ChainedEngine is a modular C++23 game engine with editor tooling, runtime packaging, ECS architecture, physics, OpenGL 4.3+ rendering, and managed C# gameplay scripting via Coral. Ships with a parkour game **Chained Decos**.

![Game Screenshot](https://i.imgur.com/MLIxRhB.png)

> [!NOTE]
> Active development is ongoing. Features and workflows continue to evolve, but this README is maintained to reflect the current repository state.

## Table of Contents

- [Overview](#overview)
- [Developer Resources (Deep Dives)](#developer-resources-deep-dives)
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
- [Project Export / Packaging](#project-export--packaging)
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

- OpenGL 4.3+ rendering pipeline with system-specific asset loader registration.
- ECS-driven scene model using EnTT.
- YAML-based project and scene serialization with deep configuration support.
- Editor workflow with hierarchy/inspector/panels and in-editor play mode.
- Flexible bootstrapping via custom Entry Points for Headless, Runtime, and Editor modes.
- Managed C# gameplay scripting through Coral (.NET/CoreCLR host).
- Project export pipeline with compressed asset packs (ZSTD) for distribution.

## Developer Resources (Deep Dives)

For a more detailed look at specific engine systems, please refer to the following guides:

- [**Engine Architecture**](docs/ARCHITECTURE.md): Bootstrapping, system initialization (SRP), and the main loop.
- [**Component Reference**](docs/COMPONENTS.md): Complete list of available ECS components and their roles.
- [**Scripting API Guide**](docs/SCRIPTING_API.md): Detailed reference for managed C# development.

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
- Asset pack export pipeline using cfnptr/pack (ZSTD compression) for runtime distribution.
- Virtual file system support is currently planned/in-progress.

## Architecture

Chained Engine follows a layered architecture with a Hazel-inspired service/singleton baseline, adapted for this project's engine/editor/runtime split.

Core layers:

- **Engine Core**: Rendering, scene management, native logic systems (e.g., transitions, hierarchy), physics, audio, and platform abstractions.
- **Bootstrapping**: Entry points that handle headless/runtime/editor initialization.
- **Editor**: Content workflows, scene inspection/manipulation, panel-based tooling.
- **Runtime**: Lightweight executable that loads and runs a project based on its `.chproject` metadata.
- **Scripting Bridge**: C++/C# interop through Coral.Native and managed assemblies.

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
   cmake -S . -B build/windows-clang -DCH_ACTIVE_GAME=testproject
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
3. **Add the entry point:** Create `game/mygame/src/main.cpp`. The engine uses a modular `Application` to bootstrap the application:
   ```cpp
   #include "engine/app/application.h"
   #include "engine/app/entry_point.h"

   namespace Chained {
       extern void RegisterGameComponents();

       Application* CreateApplication(ApplicationCommandLineArgs args) {
           RegisterGameComponents();
           ApplicationSpecification spec;
           spec.Name = "MyGame";
           spec.CommandLineArgs = args;
           spec.Headless = false;
           return new Application(spec);
       }
   }
   ```
### Project Configuration (`.chproject`)

Each game has a metadata file that defines its entry scene and title:

```yaml
Project:
  Name: Chained Decos
  AssetDirectory: assets
  ScriptsDirectory: scripts/bin
  StartScene: scenes/start_menu.chscene
```

## Project Structure

- engine/: core engine modules (graphics, scene, physics, audio, platform, assets).
- engine/network/: networking layer (ENet-based, in progress).
- editor/: ChainedEditor application and editor panels/tools.
- runtime/: ChainedRuntime application and runtime layer.
- scripting/: script host, glue bindings, and managed build integration.
- game/chaineddecos/: main game project and gameplay scripts under src/.
- game/testproject/: alternate standalone game project used for project switching and smaller experiments.
- tests/: native C++ test target (EngineTests).
- thirdparty/: third-party dependencies as git submodules.

## Dependencies

This repository relies on git submodules for core third-party libraries:

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
| zstd | ZSTD compression (shared with pack) |
| pack (cfnptr) | Asset pack creation and runtime reading |
| miniaudio | Audio |
| spdlog | Logging |
| JoltPhysics | Physics simulation |
| stb | Image loading |
| cereal | Binary serialization |
| reflect-cpp | Runtime reflection |
| ENet | UDP networking (available, not yet integrated) |

Always initialize/update submodules before configuring CMake:

```bash
git submodule update --init --recursive
```

## Prerequisites

| Tool | Version | Notes |
| :--- | :--- | :--- |
| CMake | 3.31+ | Required by top-level CMake configuration. |
| Compiler | C++23 | Clang 18+, MSVC (VS2022), or MSYS2/MinGW-w64 Clang on Windows. |
| Ninja | Latest | Recommended for fast parallel builds. |
| .NET SDK | 10.0.x | Required for managed scripting/test workflows. |
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
git clone --recurse-submodules https://github.com/IOleg-crypto/ChainedEngine.git
cd ChainedEngine
git submodule update --init --recursive
```

1. Configure (examples):

```bash
# Linux
cmake --preset linux-clang

# Windows (MSYS2 Clang)
cmake --preset windows-clang

# Windows (MSVC)
cmake --preset windows-msvc
```

1. Build:

```bash
cmake --build --preset linux-clang --parallel
cmake --build --preset windows-clang --parallel
cmake --build --preset windows-msvc --parallel
```

1. Run editor:

```bash
# Linux
./build/linux-clang/bin/ChainedEditor

# Windows (Clang)
.\build\windows-clang\bin\ChainedEditor.exe
```

## Build

CMake presets defined in CMakePresets.json:

- linux-clang
- windows-clang
- windows-msvc

Notes:

- BUILD_TESTS defaults to ON in presets.
- If you switch presets or major toolchains, do a clean configure for that build directory.
- `CH_ACTIVE_GAME` defaults to `chaineddecos` in CMakePresets.json. Set it to `testproject` when you want the alternate game.
- Optional compiler cache support is built in through ccache/sccache integration in CI and CMake options.
- If you use Clang on Windows and see Intellisense errors in VS Code, ensure that `.vscode/settings.json` points to the correct build directory for `clangd`:
  ```json
  "clangd.arguments": [
      "--compile-commands-dir=${workspaceFolder}/build/windows-clang"
  ]
  ```

## Run

Binary outputs are generated under build/{preset}/bin.

Editor:

```bash
# Linux
./build/linux-clang/bin/ChainedEditor

# Windows
.\build\windows-clang\bin\ChainedEditor.exe
```

Runtime:

```bash
# Positional project path
./build/linux-clang/bin/ChainedRuntime path/to/project.chproject

# Or explicit flag form
./build/linux-clang/bin/ChainedRuntime --project path/to/project.chproject
./build/linux-clang/bin/ChainedRuntime --project path/to/project.chproject --name "My Runtime" --width 1600 --height 900

# Windows example
.\build\windows-clang\bin\ChainedRuntime.exe --project path\to\project.chproject --name "My Runtime" --width 1600 --height 900
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
using Chained;

namespace ChainedDecos
{
    public class PlayerController : Script
    {
        public float Speed = 5.0f;

        // Lifecycle methods are public virtual on the base class; override the ones you need.
        public override void OnCreate()
        {
            // Called once, one frame after the script is instantiated.
            Log.Info("Player Controller initialized");
        }

        public override void OnUpdate(float deltaTime)
        {
            // Called every frame. Input takes the Key enum (not KeyCode).
            TransformComponent? transform = GetComponent<TransformComponent>();
            if (transform == null)
                return;

            if (Input.IsKeyDown(Key.W))
            {
                Vector3 pos = transform.Translation;
                pos.Z -= Speed * deltaTime;
                transform.Translation = pos;
            }
        }
    }
}
```

`GetComponent<T>()` returns `null` when the component is absent, so null-check the
result before using it. See [docs/SCRIPTING_API.md](docs/SCRIPTING_API.md) for the full,
verified API surface.

### Connecting the Script in the Editor

Once a script is written, wire it to an entity:

1. **Build the scripts:** Either rebuild the project through CMake/Ninja, or navigate to your `.csproj` folder and run `dotnet build`.
2. **Open the Editor** and select the entity you want to control.
3. **Add Component:** In the Inspector panel, click **Add Component** and choose **Managed Script Component**.
4. **Link it:** In the `Class Name` text field, type the **fully qualified name** of your script — namespace included (for example: `ChainedDecos.PlayerController`).
5. **Play:** Press Play in the editor. The engine instantiates the C# class and drives its lifecycle methods.

### Under the Hood: Architecture & Registration

If you are modifying the engine itself, you will find the native-to-managed bridge here:
- **Native Host:** [scripting/scriptengine.h](scripting/scriptengine.h) initializes Coral and loads assemblies.
- **Interops:** Native C++ calls are exposed to C# via `script_glue.cpp`.
- **Discovery:** At startup, `ScriptTypeRegistry::Discover()` scans the game DLL for classes deriving from `Chained.Script`.
- **Lifecycle:** `SceneScripting` instantiates the script in C++, calls `__Init()` to cache delegates, and forwards events from the native Scene to C#.

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

## Project Export / Packaging

The engine includes a project exporter that packages game assets into a single compressed archive for distribution.

### How to Export

1. Open **Editor → Project Settings → Export**.
2. Configure compression settings (or use defaults).
3. The exporter packages `.chproject`, `assets/`, and `resources/` into `resources.pack` using ZSTD compression (via [cfnptr/pack](https://github.com/cfnptr/pack)).
4. The executable, DLLs, and subdirectories are copied alongside the pack.

### Export Settings

| Setting | Default | Description |
| :--- | :--- | :--- |
| `ZipThreshold` | 0.3 | Files above this ratio of compressed/original size are stored uncompressed |
| `PreferSpeed` | false | Use faster compression (larger output) |
| `DataVersion` | 0 | Custom data version tag for the pack |

### Runtime Loading

At startup, `AssetManager::OpenPack()` automatically looks for `resources.pack` next to the executable. When found, all asset loading (textures, shaders, fonts) reads from the pack first, falling back to the filesystem if not found.

```bash
# The exported output structure looks like:
MyGame/
  MyGame.exe
  resources.pack
  engine.dll
  ...
```

## Physics and Collisions

Chained Engine uses a built-in 3D physics simulation, which is heavily used by parkour traversal scripts in `chaineddecos`.

To add physical behavior to an Entity in the Editor:
1. Click **Add Component** and select **RigidBodyComponent**. This determines if the object falls (Dynamic) or stays still (Kinematic/Static).
2. Add a physical shape like a **BoxColliderComponent** or **SphereColliderComponent**.

If you are writing a C# script (derived from `Chained.Script`), you can react to
collisions by overriding `OnCollisionEnter`. The engine passes the other entity's raw
id, not an `Entity` wrapper — construct one from the id to inspect it:

```csharp
public override void OnCollisionEnter(ulong otherEntityId)
{
    Entity other = new Entity(otherEntityId);

    TagComponent? tag = other.GetComponent<TagComponent>();
    if (tag != null)
        Log.Info($"Hit something tagged: {tag.Tag}");

    if (other.HasComponent<RigidBodyComponent>())
    {
        // Example: react to hitting a physical object.
    }
}
```

## In-Game UI

While the Editor UI is drawn using ImGui, the gameplay (In-Game) UI meant for players can be handled in two ways:

1. **Declarative Components (Recommended)**: Use `WidgetComponent` for visuals and `SceneTransitionComponent` for scene loading. This is handled natively by the engine and is the most complete way to build menus.
2. **Managed Scripting**: For custom HUD readouts, override the `OnGUI` method in your C# script and use the `UI` helper class. The managed UI surface is intentionally minimal today — it exposes a single `UI.Text` call.

```csharp
public override void OnGUI()
{
    // Lightweight HUD text. One call per line.
    UI.Text("Stamina: 100");
    UI.Text($"FPS: {Time.FPS}");
}
```

For buttons, layouts, and full menus, prefer the declarative `WidgetComponent` path
rather than drawing from script.

## Extending the Engine (C++)

Need performance that scripting can't provide, or want to create a brand new foundational Component? Here is the flow for a native ECS update:

### 1. Define the Component
Add a fast `struct` in `engine/scene/components/`. We use `EnTT`, so components are simple structs. 

```cpp
// engine/scene/components/parkour_component.h
#pragma once
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
struct ParkourComponent
{
    float Stamina = 100.0f;
    bool  IsWallRunning = false;

    static const char* GetStaticName() { return "ParkourComponent"; }

    struct UI
    {
        UIMeta Stamina = {.Min = 0.0f, .Max = 100.0f, .Speed = 1.0f};
        UIMeta IsWallRunning = {.ReadOnly = true, .Transient = true};
    };
};
CH_MARK_RFL(ParkourComponent);
} // namespace Chained
```

### 2. Register the Component
Add one line in `engine/scene/component_registry.cpp` inside `RegisterEngineComponents()`:

```cpp
RegisterReflective<ParkourComponent>("Parkour", nullptr, "Gameplay");
```

Serialization and the editor inspector work automatically through the reflection system.

### 3. Create a System
Implement the logic as a free function in a namespace:

```cpp
// engine/scene/systems/parkour_system.h
namespace Chained::Parkour {
    void Update(entt::registry& reg, Timestep ts);
}

// engine/scene/systems/parkour_system.cpp
void Parkour::Update(entt::registry& reg, Timestep ts) {
    auto view = reg.view<ParkourComponent, TransformComponent>();
    for (auto entity : view) {
        auto& [parkour, transform] = view.get<ParkourComponent, TransformComponent>(entity);
        if (parkour.IsWallRunning) {
            // Apply wallrun physics logic...
        }
    }
}
```

### 4. Hook into the Scene
Add one call in `engine/scene/scene.cpp` in the appropriate update method:

```cpp
#include "engine/scene/systems/parkour_system.h"

void Scene::OnUpdateRuntime(Timestep ts) {
    // ...existing systems...
    Parkour::Update(*m_Registry, ts);
}
```

## Debugging and Profiling

If `Chained Decos` ever suffers a frame-rate drop (a lag spike), do not optimize blindly. Use the tools:

- **Built-in Editor Profiler:** Open the **Profiler** panel. It displays a breakdown (in `ms`) of where your frame time went — `Rendering`, `Physics Update`, or `Scripting Update`. Check this first.
- **C# Debugging with CoreCLR:** Because the engine wraps .NET via Coral, you can attach a C# IDE debugger (like Visual Studio or Rider) to the running Engine/Editor process. Your breakpoints inside `OnUpdate` or `OnCreate` will pause the simulation.

## Testing

Native tests use GoogleTest + CTest and are split into two targets (see `tests/CMakeLists.txt`):

- **`engine_tests_unit`** — fast, no full engine runtime. Links only `engine_common` / `engine_core` and covers UUID, timestep, color, thread pool, events/input, layer stack, and the service locator. Sources under `tests/unit/`.
- **`engine_tests_integration`** — slower, links the full engine facade (e.g. the scripting host). Sources under `tests/integration/`.

Both register their cases with `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`, and on MinGW they statically link the gcc/stdc++ runtime (`-static-libgcc -static-libstdc++`) so discovery can launch the test exe without the MinGW DLLs on `PATH`.

```bash
# Build both test targets (default build already includes them when BUILD_TESTS=ON)
cmake --build --preset windows-clang --target engine_tests_unit --parallel
cmake --build --preset windows-clang --target engine_tests_integration --parallel

# Windows MSVC / Linux variants: swap the preset name
cmake --build --preset windows-msvc  --target engine_tests_unit --parallel
cmake --build --preset linux-clang   --target engine_tests_unit --parallel

# Run all discovered tests
ctest --test-dir build/windows-clang --output-on-failure

# Run only one layer by label (Unit / Integration)
ctest --test-dir build/windows-clang -L Unit --output-on-failure
ctest --test-dir build/windows-clang -L Integration --output-on-failure

# Windows MSVC / Linux variants
ctest --test-dir build/windows-msvc --output-on-failure
ctest --test-dir build/linux-clang  --output-on-failure
```

## CI/CD

CI workflow (.github/workflows/ci.yml) fans out to three reusable workflows:

- **Format Check** (`format.yml`): runs `clang-format-18 --dry-run -Werror` over the C++ files changed in the PR (thirdparty excluded). A single style violation fails the job, so format locally before pushing.
- **Linux Builds** (`linux.yml`): builds the `Debug` and `Release` matrix, then runs CTest under `xvfb` + Mesa software rendering (`LIBGL_ALWAYS_SOFTWARE=1`, `llvmpipe`).
- **Windows Builds** (`windows.yml`): builds the `Debug` matrix and runs CTest.

Debug configurations are built with `-DENABLE_SANITIZERS=ON` (AddressSanitizer + UndefinedBehaviorSanitizer), so use-after-free, invalid vptr, and similar memory bugs fail the test job deterministically even when the code compiles and "works" locally in Release. CTest output is captured as JUnit XML for reporting.

Deploy workflow (.github/workflows/deploy-sdk.yml):

- Triggered by v* tags or manual dispatch.
- Builds and packages ChainedEditor and ChainedRuntime artifacts for Linux/Windows.

## Troubleshooting

Submodule errors during configure/build:

```bash
git submodule update --init --recursive
```

Generator switch conflicts:

- If reusing a build directory with another generator family, reconfigure from a clean build folder for that preset.

Managed build not available:

- Ensure dotnet SDK 10.0.x is installed and available in PATH.

Headless Linux test issues:

- Install the Linux packages listed in Prerequisites.
- Use xvfb and Mesa software rendering for CI-like environments.

Changing `CH_ACTIVE_GAME` without reconfiguring the build tree can leave stale generated files behind. Reconfigure the same build directory when you switch between `chaineddecos` and `testproject`.

## Known Issues

- The font system needs rework: font rendering/handling for both in-scene (game) text and the editor is currently unreliable and is being fixed.
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
