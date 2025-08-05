# Collision Detection System

This collision detection system provides comprehensive collision detection and response for camera/player and 3D models in your Raylib-based game engine.

## Features

- **Collision Shapes**: Support for spheres and axis-aligned bounding boxes (AABB)
- **Collision Detection**: Sphere-sphere, AABB-AABB, and sphere-AABB collision detection
- **Collision Response**: Automatic collision response with position correction
- **Debug Visualization**: Visual debugging tools for collision bounds
- **Integration**: Easy integration with Player and ModelInstance classes

## Usage

### Basic Setup

1. **Include the collision system**:
```cpp
#include <Collision/CollisionSystem.h>
```

2. **Player collision is automatically initialized** when you create a Player object:
```cpp
auto player = std::make_shared<Player>();
// Player collision component is automatically set up with a sphere collider
```

3. **Model collision is automatically initialized** when you create ModelInstance objects:
```cpp
auto model = std::make_unique<ModelInstance>(position, modelPtr, scale, name);
// ModelInstance collision component is automatically set up with AABB collider
```

### Collision Detection

#### Manual Collision Checking
```cpp
// Check if player collides with a model
if (player->CheckCollision(model->GetCollisionComponent())) {
    // Handle collision
    player->HandleCollisionResponse(model->GetCollisionComponent());
}
```

#### Automatic Collision Handling
```cpp
// Create a list of colliders (excluding the player)
std::vector<CollisionComponent*> colliders;
for (auto& model : models) {
    colliders.push_back(&model->GetCollisionComponent());
}

// Check and handle all collisions automatically
player->CheckAndHandleCollisions(colliders);
```

#### Movement with Collision Detection
```cpp
Vector3 moveOffset = {1.0f, 0.0f, 0.0f};
if (player->MoveWithCollisionDetection(moveOffset, colliders)) {
    // Movement was successful
} else {
    // Movement was blocked by collision
}
```

### Collision Types

#### Sphere Collision
```cpp
// Player uses sphere collision by default
CollisionComponent& playerCollision = player->GetCollisionComponent();
playerCollision.type = CollisionComponent::SPHERE;
playerCollision.sphere.radius = 0.5f;
```

#### AABB Collision
```cpp
// Models use AABB collision by default
CollisionComponent& modelCollision = model->GetCollisionComponent();
modelCollision.type = CollisionComponent::AABB;
model->SetCollisionSize({2.0f, 2.0f, 2.0f});
```

#### Both Types
```cpp
// Use both sphere and AABB for more precise collision
collisionComponent.type = CollisionComponent::BOTH;
```

### Debug Visualization

```cpp
// Enable debug visualization
CollisionSystem::DrawCollisionSphere(player->GetCollisionComponent().sphere, RED);
CollisionSystem::DrawCollisionBox(model->GetCollisionComponent().box, GREEN);
```

### Example Integration in Game Loop

```cpp
void GameUpdate() {
    // Update player (includes collision bounds update)
    player->Update();
    
    // Collect all static colliders
    std::vector<CollisionComponent*> staticColliders;
    for (auto& model : worldModels) {
        if (model->GetCollisionComponent().isStatic) {
            staticColliders.push_back(&model->GetCollisionComponent());
        }
    }
    
    // Handle collisions
    player->CheckAndHandleCollisions(staticColliders);
}

void GameDraw() {
    // Draw your game objects
    DrawModels();
    
    // Draw collision debug visualization (optional)
    if (showDebugCollision) {
        CollisionSystem::DrawCollisionSphere(player->GetCollisionComponent().sphere, RED);
        for (auto& model : worldModels) {
            CollisionSystem::DrawCollisionBox(model->GetCollisionComponent().box, GREEN);
        }
    }
}
```

## Collision Component Properties

- `type`: CollisionComponent::SPHERE, AABB, or BOTH
- `sphere`: CollisionSphere with center and radius
- `box`: CollisionBox with min and max points
- `isStatic`: True for immovable objects (like walls, buildings)
- `isActive`: True to enable collision detection for this component

## Performance Tips

1. Use `isActive = false` to temporarily disable collision for objects
2. Use `isStatic = true` for immovable objects to optimize collision response
3. Use appropriate collision shapes (sphere for characters, AABB for buildings)
4. Consider using spatial partitioning for large numbers of objects

## Customization

### Custom Collision Shapes
You can modify collision bounds manually:
```cpp
// Custom sphere size
model->SetCollisionRadius(1.5f);

// Custom AABB size
model->SetCollisionSize({3.0f, 2.0f, 1.0f});

// Custom collision type
model->SetCollisionType(CollisionComponent::SPHERE);
```

### Custom Collision Response
Override the collision response behavior by modifying the `HandleCollisionResponse` method or implementing your own collision handling logic.