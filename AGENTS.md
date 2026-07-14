# AGENTS.md

## Build

Submodules required: `git submodule update --init --recursive`

Presets: `windows-clang`, `windows-msvc`, `linux-clang`, `linux-gcc`.

```bash
cmake --preset windows-clang
cmake --build --preset windows-clang --parallel
```

Binaries: `build/{preset}/bin/` — `ChainedEditor`, `ChainedRuntime`, game executable.

**Key CMake variables:**
- `CH_ACTIVE_GAME` — `chaineddecos` (default) or `testproject`. **Build-time only.** Changing it on an existing build dir requires reconfigure, not just rebuild. Stale generated files persist otherwise.
- `BUILD_TESTS` — ON by default.
- `CH_ENGINE_SHARED` — OFF by default (static engine).

## Tests

```bash
cmake --build --preset windows-clang --target EngineTests --parallel
ctest --test-dir build/windows-clang --output-on-failure
```

Tests split into `tests/unit/` (fast, no engine runtime) and `tests/integration/` (slow, need engine + DLLs). CI runs both; integration tests may be gated/skipped depending on environment.

Managed (C#) tests require .NET SDK 10.0.x on PATH. Without it, the scripting target is silently skipped.

Linux CI tests run under `xvfb` with Mesa software rendering (`LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`).

## Formatting

C++: `.clang-format` — LLVM base, 4-space indent, 120-col limit, Allman braces, left-aligned pointers. CI uses `clang-format-18` specifically. Run locally:

```bash
clang-format-18 -i --style=file <files>
```

YAML: CI runs `yamllint -d relaxed` on changed `.yml`/`.yaml` files.

CI format check only runs on files changed relative to the PR base — not the entire repo.

## Architecture (what agents get wrong)

**Engine is a static lib facade.** Subdirectories under `engine/` build as separate CMake modules (`engine_core`, `engine_graphics`, etc.) but are linked into a single `engine` target. Games/editor/runtime link `ChainedEngine::Framework`, not individual modules.

**Service initialization order is implicit.** `Application` registers services (window, ThreadPool, ComponentSerializer, AssetManager, Renderer, etc.) in declaration order. Reordering is a footgun — see `docs/ARCHITECTURE.md` §5.

**Two UI systems, don't conflate them:**
- Editor UI = ImGui (immediate-mode, editor-only)
- In-game UI = `WidgetComponent` + `SceneTransitionComponent` (native) or C# `Script.OnGUI()` via `scripting/managed/src/UI.cs`

**Game selection is two mechanisms:**
- `CH_ACTIVE_GAME` CMake var = build-time (which game folder compiles)
- `.chproject` YAML = runtime (which scene/assets the editor/runtime loads)

**Adding a native component:** struct in `engine/scene/components.h`, YAML in `scene_serializer.cpp`, inspector in `editor/editor_panels.cpp`, logic in a `SceneSystem`. See `docs/COMPONENTS.md`.

**Scripting bridge:** C++/C# via Coral (.NET). `ScriptTypeRegistry::Discover()` scans game assembly for `Chained.Script` subclasses. `SceneScripting` instantiates and calls `__Init()`. Managed API: `scripting/managed/src/` (`Script.cs`, `Entity.cs`, `Input.cs`, `UI.cs`, etc.).

## CI

Workflow: `.github/workflows/ci.yml` dispatches to `format.yml`, `linux.yml`, `windows.yml`.

Triggers: push/PR to `main`, `develop`, `opengl` branches on C++/C#/CMake/YAML changes.

`CH_CI=ON` is set in CI — code can branch on this define.

Resource sync runs post-build via `tools/sync_resources.py` (Python required at configure time).

## Gotchas

- `clangd` in VS Code defaults to `build/linux-clang` for compile commands (see `.clangd`). Update if using a different preset.
- MSVC builds use static CRT (`/MT`) — set globally before `project()`.
- MinGW builds need `-Wa,-mbig-obj` for large translation units.
- Virtual file system is planned/in-progress — don't assume it exists.
- `network/` module is commented out in `engine/CMakeLists.txt`.
