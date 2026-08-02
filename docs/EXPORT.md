# Project Export / Packaging

The engine includes a project exporter that packages game assets into a single compressed archive for distribution.

## Table of Contents

- [Scene Serialization Format](#scene-serialization-format)
- [Debug Settings in Scene](#debug-settings-in-scene)
- [How to Export](#how-to-export)
- [Export Settings](#export-settings)
- [Runtime Loading](#runtime-loading)

---

## Scene Serialization Format

Scenes are stored as YAML (`.chscene` files). Each entity is serialized with its UUID, and components are nested under their serialization keys.

```yaml
Scene:
  Entities:
    - Entity: 1234567890          # UUID as uint64
      TagComponent:
        Tag: "Player"
      NameComponent:
        Name: "Player Entity"
      TransformComponent:
        Translation: [0.0, 5.0, 0.0]
        Rotation: [0.0, 0.0, 0.0]
        Scale: [1.0, 1.0, 1.0]
      RigidBodyComponent:
        Type: 1                    # 0=Static, 1=Dynamic, 2=Kinematic
        Mass: 1.0
        UseGravity: true
      ColliderComponent:
        Type: 0                    # 0=Box, 1=Sphere, 2=Capsule, 3=Mesh
        Size: [0.5, 0.5, 0.5]
        Friction: 0.5
      ModelComponent:
        ModelPath: "assets/models/player.glb"
      AudioComponent:
        SoundPath: "assets/sounds/footstep.wav"
        Spatialized: true
        Volume: 0.8
    - Entity: 9876543210
      TagComponent:
        Tag: "SpawnPoint"
      SpawnComponent:
        IsActive: true
        SpawnPoint: [0.0, 1.0, 0.0]
        ZoneSize: [5.0, 2.0, 5.0]
      Hierarchy:
        Parent: 0                  # 0 = root entity
        Children: []
```

## Debug Settings in Scene

Scene-level debug rendering is serialized under `DebugSettings`:

```yaml
DebugSettings:
  DiagnosticMode: 0               # 0=Full, 1=Normals, 2=Lighting, 3=Albedo
  DrawColliders: false
  DrawHierarchy: false
  DrawGrid: false
  DrawSelection: true
  DrawLights: true
  DrawSpawnZones: true
  CollisionWireframeMode: 0       # 0=Wireframe, 1=Solid, 2=Solid+Wireframe
Grid:
  Spacing: 1.0
```

## How to Export

1. In the editor, go to **File > Export Project**.
2. Choose a **Pack Mode**:
   - **Fast (LZ4)** — Quick export, larger file.
   - **Balanced (ZSTD)** — Slower export, smaller file.
   - **Raw** — No compression.
3. Adjust **Compression Threshold** if needed.
4. Click **Browse Output Folder** and select a destination.
5. The export starts automatically. Progress is shown in the overlay.

The exporter packages `.chproject`, `assets/`, and `resources/` into `resources.pack` using compression (via [cfnptr/pack](https://github.com/cfnptr/pack)). The executable, DLLs, and subdirectories are copied alongside the pack.

## Export Settings

| Setting | Default | Description |
| :--- | :--- | :--- |
| `Pack Mode` | Balanced | Compression algorithm: Fast (LZ4), Balanced (ZSTD), or Raw (none) |
| `ZipThreshold` | 0.05 | Files above this ratio of compressed/original size are stored uncompressed |
| `DataVersion` | 0 | Increment to invalidate cached packs at runtime |

## Runtime Loading

At startup, `AssetManager::OpenPack()` automatically looks for `resources.pack` next to the executable. When found, all asset loading (textures, shaders, fonts) reads from the pack first, falling back to the filesystem if not found.

```
# The exported output structure looks like:
MyGame/
  MyGame.exe
  resources.pack
  engine.dll
  ...
```

---

For build instructions, see [Build](../readme.md#build).
For the User Guide, see [User Guide](USER_GUIDE.md).
