# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Always initialize submodules before first configure:
```bash
git submodule update --init --recursive
```

**Configure:**
```bash
cmake --preset windows-clang   # Windows with Clang (primary)
cmake --preset windows-msvc    # Windows with MSVC
cmake --preset linux-clang     # Linux
```

**Build:**
```bash
cmake --build --preset windows-clang --parallel
cmake --build --preset windows-msvc --parallel
cmake --build --preset linux-clang --parallel

# Specific config (Debug/Release/RelWithDebInfo):
cmake --build --preset windows-clang-debug --parallel
cmake --build --preset windows-clang-release --parallel
```

**Binaries** land in `build/{preset}/bin/`.

**Run editor:**
```bash
.\build\windows-clang\bin\ChainedEditor.exe        # Windows
./build/linux-clang/bin/ChainedEditor              # Linux
```

**Run runtime:**
```bash
.\build\windows-clang\bin\ChainedRuntime.exe --project path\to\game.chproject --width 1920 --height 1080
```

**Build and run tests:**
```bash
cmake --build --preset windows-clang --target EngineTests --parallel
ctest --test-dir build/windows-clang --output-on-failure

# Run a single test binary directly:
.\build\windows-clang\bin\EngineTests.exe --gtest_filter=SceneTests.*
```

**Switch active game project** (default is `chaineddecos`):
```bash
cmake --preset windows-clang -DCH_ACTIVE_GAME=testproject
```

**Sync resources manually** (normally runs as post-build step):
```bash
python tools/sync_resources.py sync-resources --root . --bin build/windows-clang/bin
```

**Build C# scripts only:**
```bash
cd game/chaineddecos/scripts && dotnet build
```

## Architecture

### Layers

```
Application (engine/app/application.h)
  └── LayerStack  →  pushes engine layers + editor/runtime layer
        ├── ImGuiLayer
        └── EditorLayer / RuntimeLayer (per executable)

ServiceLocator (engine/core/service_locator.h)
  └── owns all EngineModules: Renderer, Physics, AudioSystem, AssetManager, ScriptEngine, ...
      Initialize/Update/Shutdown called in registration order (reversed for shutdown)
```

`EngineModule` is the base class for every engine service. All services are accessed globally via `ServiceLocator::Get<T>()`. Use `TryGet<T>()` when the service may be absent. Services are registered before `ServiceLocator::Lock()` is called; no new services can be added after that.

### Scene and ECS

- `Scene` owns an `entt::registry` and coordinates all in-scene systems.
- Scene states: `Edit` → `Runtime` → `Simulation`. State transitions via `Scene::TransitionToState()`.
- Components are plain structs defined in `engine/scene/components/` and aggregated in `engine/scene/components.h`.
- Systems live in `engine/scene/systems/` and `engine/scene/hierarchy_system.h`. Scene-level systems (physics, scripting, animation) are held as members of `Scene`.
- Serialization: `SceneSerializer` + `ComponentSerializer` + `HierarchySerializer` write/read YAML. When adding a new component, update all three plus the editor inspector in `editor/`.

### Rendering Pipeline

`SceneRenderer` orchestrates four sequential render passes per frame:

1. **ShadowPass** — depth-only pass; builds `LightSpaceMatrix` from the directional `LightComponent`. Result propagated to `RendererData` after the pass completes.
2. **SkyboxPass** — renders the environment background.
3. **GeometryPass** — draws opaque queue, then transparent queue (back-to-front sorted). Calls `BindShaderUniforms` → `Renderer::SetLightingUniforms` per draw, which uploads shadow state, SSBO lights, and fog.
4. **CompositePass** — post-process / fog composite.

`Renderer` (singleton `EngineModule`) owns the SSBO, UBOs, shader library (`ShaderStorage`), and static mesh resources. `SceneRenderer` is a non-singleton orchestrator instantiated per-viewport.

Key flow: `SceneRenderer::PrepareLights()` fills the SSBO → `Renderer::BeginScene()` uploads it to GPU → passes execute → `Renderer::EndScene()`.

### Shaders

System shaders live in `resources/shaders/`. Structure:
- `include/` — shared GLSL modules (`lighting_common.glsl`, `shadow.glsl`, `fog.glsl`, `tonemap.glsl`, etc.)
- `materials/` — `lighting.vs/.fs`, `skinned.vs`, `unlit.fs`, `shadow_depth.vs/.fs`
- `env/` — skybox variants
- `postfx/` — `post_process.vs/.fs`
- `debug/` — grid, collider overlays

The lighting shader uses a SSBO at binding 0 for up to 256 dynamic lights. The global directional light (sun) comes from `LightingSettings` uploaded as uniforms (`lightDir`, `lightColor`, `ambient`).

### Asset System

`AssetManager` (service) loads assets by path, deduplicates by UUID. Asset types (`ModelAsset`, `TextureAsset`, `ShaderAsset`, `EnvironmentAsset`, etc.) live in `engine/assets/types/`. Loaders are in `engine/assets/loaders/`. Assets go through states: `Unloaded → Loading → Ready`.

### C# Scripting

- `ScriptEngine` (service, `scripting/scriptengine.h`) hosts Coral/CoreCLR and loads game DLLs.
- `script_glue_*.cpp` files expose C++ APIs to managed code as internal calls.
- `SceneScriptingManager` instantiates scripts per-entity and drives `OnCreate`/`OnUpdate`/`OnDestroy`/`OnCollision` lifecycle.
- The managed base class is `scripting/managed/src/Script.cs`. Game scripts inherit from `Chained.Script` and are linked via `ScriptComponent` (class name stored as string in the component).

### Project Configuration

- `.chproject` YAML file: defines `StartScene`, `AssetDirectory`, `ScriptsDirectory`.
- `.chenv` YAML file: environment/lighting settings (`EnvironmentAsset`).
- `.chscene` YAML file: scene entity data.
- `CH_ACTIVE_GAME` CMake variable selects which `game/` subdirectory is built. Only one game is compiled at a time.

### Adding a New Component

1. Add struct to `engine/scene/components/` and include in `engine/scene/components.h`.
2. Register in `engine/scene/component_registry.cpp`.
3. Add YAML read/write in `engine/scene/component_serializer.cpp`.
4. Add ImGui inspector UI in the editor panel for that component type.
5. If it needs a system, add a `SceneSystem` in `engine/scene/systems/` and register it in `Scene`.

## Key Files

| File | Role |
|---|---|
| `engine/core/service_locator.h` | Global service registry |
| `engine/app/application.h` | App entry point and main loop |
| `engine/scene/scene.h` | ECS scene container |
| `engine/scene/components.h` | All component types aggregated |
| `engine/graphics/pipeline/renderer.h` | Low-level draw calls, SSBO, UBO |
| `engine/graphics/pipeline/scene_renderer.h` | High-level scene render orchestrator |
| `engine/assets/asset_manager.h` | Asset loading and deduplication |
| `scripting/scriptengine.h` | Coral/CoreCLR script host |
| `scripting/script_glue.cpp` | C++ → C# internal call bindings |
| `resources/shaders/materials/lighting.fs` | Main PBR fragment shader |
| `resources/shaders/include/lighting_common.glsl` | Light SSBO and shared uniforms |
| `engine/app/entry_point.h` | `main()` macro; calls `CreateApplication()` |
