# Component Reference

Chained Engine uses a pure ECS (Entity Component System) architecture powered by **EnTT**. Below is a list of the most commonly used components.

## Core Components

### `IDComponent`
*   **Description**: Stores the globally unique identifier (UUID) for an entity.
*   **Properties**: `ID` (UUID).

### `TagComponent`
*   **Description**: A human-readable name for searching and identification in the editor.
*   **Properties**: `Tag` (string).

### `TransformComponent`
*   **Description**: Defines the position, rotation, and scale of the entity in 3D space.
*   **Properties**: `Translation`, `Rotation` (Euler angles), `Scale`.

### `HierarchyComponent`
*   **Description**: Manages parent-child relationships between entities.
*   **Properties**: `Parent` (UUID), `Children` (list of UUIDs).

## Rendering Components

### `MeshComponent`
*   **Description**: Attaches a 3D model (static or dynamic) to the entity.
*   **Properties**: `AssetHandle` (UUID), `MaterialOverrides`.

### `CameraComponent`
*   **Description**: Defines a viewpoint for rendering.
*   **Properties**: `ProjectionType` (Perspective/Orthographic), `FOV`, `Near/Far Clips`.

### `LightComponent`
*   **Description**: Emits light into the scene.
*   **Properties**: `Type` (Point, Spot, Directional), `Color`, `Intensity`, `Radius`.

## Physics Components

### `RigidBodyComponent`
*   **Description**: Subjects the entity to physics simulation.
*   **Properties**: `BodyType` (Static, Dynamic, Kinematic), `Mass`, `Linear/Angular Damping`.

### `BoxColliderComponent` / `SphereColliderComponent`
*   **Description**: Defines the physical shape for collisions.
*   **Properties**: `Size`, `Offset`, `IsTrigger`, `Friction`.

## Gameplay & Logic

### `ManagedScriptComponent`
*   **Description**: Links the entity to one or more C# scripts.
*   **Properties**: `ClassName` (Fully qualified, e.g., `MyGame.Player`).

### `AudioComponent`
*   **Description**: Handles 3D spatialized sound.
*   **Properties**: `AssetHandle` (UUID), `Volume`, `Pitch`, `PlayOnAwake`, `Looping`.
