# Scripting API Reference (C#)

This document covers the managed API surface available to C# scripts in Chained Engine.

## 1. Script Lifecycle

All gameplay scripts must inherit from `Chained.Script`.

```csharp
public class MyScript : Script
{
    protected override void OnCreate() { /* Called once when the entity is instantiated */ }
    protected override void OnUpdate(float deltaTime) { /* Called every frame */ }
    protected override void OnDestroy() { /* Called when the entity is removed */ }
    protected override void OnCollisionEnter(Entity other) { /* Physics callback */ }
    protected override void OnGUI() { /* Immediate-mode UI layout */ }
}
```

## 2. Entity & Component Access

### Local Access
*   `Entity self`: Access the entity this script is attached to.
*   `T GetComponent<T>()`: Retrieves a component from the current entity.
*   `bool HasComponent<T>()`: Checks if the entity has a specific component.

### Global Search
*   `Entity Scene.FindEntityByTag(string tag)`: Find an entity by its name/tag.
*   `Entity Scene.CreateEntity(string name)`: Spawn a new entity.
*   `void Scene.DestroyEntity(Entity entity)`: Remove an entity.

## 3. Input Handling

Use the static `Input` class to query hardware state:
*   `bool Input.IsKeyDown(KeyCode code)`: Check keyboard.
*   `bool Input.IsMouseButtonPressed(MouseButton button)`: Check mouse.
*   `Vector2 Input.GetMousePosition()`: Screen-space coordinates.

## 4. Mathematics

The engine uses custom wrappers for GLM types:
*   `Vector2`, `Vector3`, `Vector4`: Standard coordinate and color containers.
*   `Quaternion`: For stable rotation math.
*   `Math.Lerp`, `Math.Clamp`: Common scalar utilities.

## 5. UI & Logging

### UI (In-Game HUD)
Draw simple debug or gameplay UI inside `OnGUI`:
*   `void UI.DrawText(string text, Vector2 pos, Color color)`
*   `bool UI.DrawButton(string label, Vector2 pos)`

### Logging
*   `Log.Trace`, `Log.Info`, `Log.Warn`, `Log.Error`: Print messages to the engine console and editor log.
