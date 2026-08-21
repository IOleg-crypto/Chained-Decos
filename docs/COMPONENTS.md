# Component Reference

Chained Engine uses a pure ECS (Entity Component System) architecture powered by **EnTT**. Below is a list of the most commonly used components with examples.

## Core Components

### `IDComponent`
- **Description**: Stores the globally unique identifier (UUID) for an entity.
- **Properties**: `ID` (UUID)
- **YAML Example**:

  ```yaml
  IDComponent:
    ID: 13765047633248252361
  ```

- **C++ Example**:

  ```cpp
  auto uuid = entity.GetComponent<IDComponent>().ID;
  ```

### `TagComponent`
- **Description**: A human-readable name for searching and identification.
- **Properties**: `Tag` (string)
- **YAML Example**:

  ```yaml
  TagComponent:
    Tag: Main Player
  ```

- **C++ Example**:

  ```cpp
  entity.GetComponent<TagComponent>().Tag = "New Name";
  ```

### `TransformComponent`
- **Description**: Defines the position, rotation, and scale.
- **Properties**: `Translation`, `Rotation` (Euler), `Scale`
- **YAML Example**:

  ```yaml
  TransformComponent:
    Translation: [0, 5, -10]
    Rotation: [0, 90, 0]
    Scale: [1, 1, 1]
  ```

- **C++ Example**:

  ```cpp
  auto& transform = entity.GetComponent<TransformComponent>();
  transform.Translation.y += 1.0f;
  ```

## Rendering Components

### `MeshComponent`
- **Description**: Attaches a 3D model to the entity.
- **Properties**: `AssetHandle` (UUID), `MaterialOverrides`
- **YAML Example**:

  ```yaml
  MeshComponent:
    AssetHandle: models/player.glb
  ```

- **C++ Example**:

  ```cpp
  entity.AddComponent<MeshComponent>("path/to/model.glb");
  ```

### `CameraComponent`
- **Description**: Defines a viewpoint for rendering.
- **Properties**: `ProjectionType`, `FOV`, `Near/Far Clips`
- **YAML Example**:

  ```yaml
  CameraComponent:
    ProjectionType: 0
    PerspectiveFOV: 45
    PerspectiveNear: 0.1
    PerspectiveFar: 1000
  ```

## UI Components

### `WidgetComponent`
- **Description**: The base for all in-game UI (buttons, text, images).
- **Properties**: `WidgetType`, `Label`, `BoxStyle`, `TextStyle`
- **YAML Example**:

  ```yaml
  WidgetComponent:
    Widget Type: 1 # Button
    Label: Start Game
    Box Style:
      BG Color: [40, 40, 40, 255]
      Rounding: 4
  ```

## Gameplay & Logic

### `SceneTransitionComponent`
- **Description**: Triggers a scene change. Automatically detects clicks if a `WidgetComponent` is present on the same entity.
- **Properties**: `TargetScenePath` (string), `Triggered` (bool)
- **YAML Example**:

  ```yaml
  Scene TransitionComponent:
    Target Scene Path: scenes/level1.chscene
    Triggered: false
  ```

- **C++ Example**:

  ```cpp
  // Manual trigger
  entity.GetComponent<SceneTransitionComponent>().Triggered = true;
  ```

### `ManagedScriptComponent`
- **Description**: Links the entity to C# scripts.
- **Properties**: `ClassName`
- **YAML Example**:

  ```yaml
  ManagedScriptComponent:
    Scripts:
      - ClassName: ChainedDecos.PlayerController
  ```

### `AudioComponent`
- **Description**: Handles 3D spatialized sound.
- **Properties**: `AssetHandle`, `Volume`, `Looping`
- **YAML Example**:

  ```yaml
  AudioComponent:
    AssetHandle: audio/ambient.wav
    Volume: 0.5
    Looping: true
  ```

---

## Adding a Native Component (C++)

Need performance that scripting can't provide, or want to create a brand new foundational component? Here is the full workflow:

### 1. Define the Component

Add a fast `struct` in `engine/scene/components/`. We use `EnTT`, so components are plain structs — no methods on the component itself.

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

Implement the logic as a free function in a namespace. **Always use `TransformSystem::` functions** to read/write transforms — never access `TransformComponent` fields directly.

```cpp
// engine/scene/systems/parkour_system.h
#pragma once
#include "engine/scene/scene_fwd.h"

namespace Chained::Parkour
{
    void Update(entt::registry& reg, Timestep ts);
}

// engine/scene/systems/parkour_system.cpp
#include "parkour_system.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "engine/scene/systems/transform_system.h"

namespace Chained::Parkour
{
    void Update(entt::registry& reg, Timestep ts)
    {
        auto view = reg.view<ParkourComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto& parkour = view.get<ParkourComponent>(entity);
            auto& tc = view.get<TransformComponent>(entity);

            if (parkour.IsWallRunning)
            {
                // Use TransformSystem — never write tc.Translation directly
                auto pos = TransformSystem::GetTranslation(tc);
                pos.y += 2.0f * ts;
                TransformSystem::SetTranslation(tc, pos);
            }
        }
    }
}
```

> **Note**: `component_utils.h` was removed. All transform utilities live in `TransformSystem::` — see `engine/scene/systems/transform_system.h` for the full list (ComputeLocalMatrix, GetTranslation, GetRotation, GetScale, SetTranslation, SetRotation, SetScale, etc.).

### 4. Hook into the Scene

Add one call in `engine/scene/scene.cpp` in the appropriate update method:

```cpp
#include "engine/scene/systems/parkour_system.h"

void Scene::OnUpdateRuntime(Timestep ts)
{
    // ...existing systems...
    Parkour::Update(*m_Registry, ts);
}
```

### 5. Expose to Scripting (C# Glue)

To make the component's fields accessible from C# scripts, add a `[NativeProperty]` attribute on the C# wrapper class. The C++ glue code is generated automatically.

**Step A — Create the C# wrapper** (if it doesn't exist):

```csharp
// engine/scripting/managed/src/Components/ParkourComponent.cs
using System;

namespace Chained
{
    [NativeProperty("Stamina", "float", "ParkourComponent_GetStamina", "ParkourComponent_SetStamina")]
    [NativeProperty("IsWallRunning", "bool", "ParkourComponent_IsWallRunning", "ParkourComponent_SetIsWallRunning")]
    public partial class ParkourComponent : Component
    {
    }
}
```

**Step B — Register the class for glue generation** in `engine/scripting/CMakeLists.txt`:

```cmake
--classes PlayerComponent SpawnComponent NetworkIdentityComponent ParkourComponent
```

**Step C — Build**:

```bash
cmake --build --preset windows-clang-debug --parallel
```

The `tools/generate_glue.py` script runs automatically during the build and generates:
- `engine/scripting/generated/script_glue_generated.h` — function declarations
- `engine/scripting/generated/script_glue_generated.cpp` — getter/setter implementations
- `engine/scripting/generated/script_glue_generated_reg.cpp` — `AddInternalCall` registrations

No manual C++ glue code is needed for simple property get/set. For complex logic (physics sync, string conversion, etc.), use `[NativeCall]` instead and write hand-written glue in `script_glue_*.cpp` — see [Scripting Interop](SCRIPTING_INTEROP.md).
