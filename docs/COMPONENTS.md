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
