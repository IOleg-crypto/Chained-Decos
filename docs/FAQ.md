# FAQ / Common Patterns

Frequently asked questions and common patterns for ChainedEngine.

## Table of Contents

- [Scripting](#scripting)
- [Editor](#editor)
- [Build & Setup](#build--setup)
- [Components](#components)

---

## Scripting

### How do I teleport a player to a spawn point?

Use `ForceSetVelocity` (not `Velocity =`) to avoid Jolt's Dynamic body Y-velocity override:

```csharp
public void TeleportToSpawn(Vector3 spawnPos) {
    TransformComponent? transform = GetComponent<TransformComponent>();
    RigidBodyComponent? rb = GetComponent<RigidBodyComponent>();
    if (transform != null) transform.Translation = spawnPos;
    if (rb != null) rb.ForceSetVelocity(Vector3.Zero);  // Not rb.Velocity = ...
}
```

### How do I add a component from a C# script?

```csharp
public override void OnCreate() {
    RigidBodyComponent rb = entity.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent.BodyType.Dynamic;
    rb.Mass = 1.5f;
}
```

### How do I respond to collisions?

Override `OnCollisionEnter` in your script. The engine passes a raw entity ID, not an `Entity` wrapper:

```csharp
public override void OnCollisionEnter(ulong otherEntityId) {
    Entity other = new Entity(otherEntityId);
    TagComponent? tag = other.GetComponent<TagComponent>();
    if (tag != null && tag.Tag == "Pickup") {
        Log.Info("Collected a pickup!");
    }
}
```

### How do I trigger a scene transition?

Add a `SceneTransitionComponent` to your entity and set `TargetScenePath`. When `Triggered` is set to `true` (by a script or collision), the engine loads the target scene automatically.

### How do I play spatial audio?

Add an `AudioComponent` with `Spatialized = true` and `PlayOnStart = true`. The audio system syncs the listener with the primary camera automatically.

### How do I switch between scenes from a script?

```csharp
Scene.LoadScene("assets/scenes/level2.chscene");
```

### How do I find an entity by name/tag?

```csharp
Entity? player = Scene.FindEntityByTag("Player");
if (player != null)
{
    TransformComponent? t = player.GetComponent<TransformComponent>();
}
```

### How do I copy/duplicate an entity?

```csharp
Entity? clone = Scene.CopyEntity(original);
```

### How do I exit the game from a script?

```csharp
Application.Close();
```

### How do I get the FPS or delta time?

```csharp
int fps = Time.FPS;
float dt = Time.DeltaTime;
```

### How do I change the window size at runtime?

```csharp
AppWindow.SetSize(1920, 1080);
AppWindow.SetFullscreen(true);
```

Or configure it in the `.chproject` file:

```yaml
Project:
  Window:
    Width: 1920
    Height: 1080
    VSync: true
```

### How do I add sound/music?

1. Place your `.wav` or `.mp3` files in the assets folder.
2. Create an entity, add **AudioComponent**.
3. Set the audio file path in the Inspector.
4. Enable **PlayOnStart** for automatic playback, or call `Audio.Play("path/to/sound.wav")` from a script.

For background music, enable **Loop** on the component.

---

## Editor

### My script doesn't compile. What's wrong?

Most C# script errors come from:

1. **Wrong namespace.** Your script must be in a namespace inside the game assembly (e.g., `namespace ChainedDecos`).
2. **Missing `using Chained;`** — Required for `Script`, `Entity`, `Input`, `Log`, etc.
3. **.NET SDK not found.** Install .NET SDK 10.0.x and verify it's on PATH: `dotnet --version`.
4. **Script not in the right folder.** Place `.cs` files in `game/<yourgame>/assets/scripts/src/`.

### The editor doesn't see my script. What do I do?

After adding a new script file, the script assembly needs to rebuild. The engine auto-rebuilds when you press **Play**, but if it doesn't:

1. Save the script file.
2. In the editor, go to **File > Reload Scripts** (or press Ctrl+R).
3. Check the **Console** panel for compilation errors.

---

## Build & Setup

### How do I switch between game projects?

Set `CH_ACTIVE_GAME` at CMake configure time:

```bash
cmake -S . -B build/windows-clang -DCH_ACTIVE_GAME=testproject
```

The executable name changes automatically. The `.chproject` file determines what scene the runtime opens.

### What build presets are available?

| Preset | Compiler | Platform |
|---|---|---|
| `windows-clang-debug` | Clang | Windows |
| `windows-clang-release` | Clang | Windows |
| `windows-msvc-debug` | MSVC | Windows |
| `windows-msvc-release` | MSVC | Windows |
| `windows-gcc-debug` | GCC | Windows (MinGW) |
| `windows-gcc-release` | GCC | Windows (MinGW) |
| `linux-clang-debug` | Clang | Linux |
| `linux-clang-release` | Clang | Linux |
| `linux-gcc-debug` | GCC | Linux |
| `linux-gcc-release` | GCC | Linux |

### How do I run tests?

```bash
cmake --build --preset windows-clang-debug --target EngineTests --parallel
ctest --test-dir build/windows-clang-debug --output-on-failure
```

### The build fails with submodule errors

Run this before building:

```bash
git submodule update --init --recursive
```

### How do I add a new component in C++?

See the [Component Reference](COMPONENTS.md) for the full guide. In short:

1. Define a struct in `engine/scene/components/`.
2. Register it in `component_registry.cpp`.
3. Add a system in `engine/scene/systems/`.
4. Hook the system into the scene update loop.
