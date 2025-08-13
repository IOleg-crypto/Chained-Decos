# Collision Detection System

This advanced collision detection system provides comprehensive collision detection and response for camera/player and 3D models in your Raylib-based game engine. It supports both traditional AABB collision detection and precise BVH (Bounding Volume Hierarchy) collision for complex meshes.

## Features

### Traditional Collision Detection
- **Collision Shapes**: Support for spheres and axis-aligned bounding boxes (AABB)
- **Collision Detection**: Sphere-sphere, AABB-AABB, and sphere-AABB collision detection
- **Collision Response**: Automatic collision response with position correction
- **Debug Visualization**: Visual debugging tools for collision bounds
- **Integration**: Easy integration with Player and ModelInstance classes

### Advanced BVH Collision System
- **Precise Mesh Collision**: Triangle-level collision detection using BVH trees
- **Ray Casting**: High-performance ray-triangle intersection for shooting, picking, line-of-sight
- **Spatial Hierarchy**: Efficient collision queries for complex 3D models
- **Model-to-Model**: Precise collision between complex geometric shapes
- **Point-in-Mesh**: Accurate containment testing
- **Performance Optimization**: Hierarchical culling for fast collision queries

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

---

## BVH (Bounding Volume Hierarchy) System

The BVH system provides precise triangle-level collision detection for complex 3D models. It's ideal for detailed meshes where AABB collision is too imprecise.

### BVH Setup

#### Basic BVH Creation
```cpp
#include <Collision/CollisionSystem.h>

// Create collision object
Collision collision;

// Load model and build BVH
Model complexModel = LoadModel("resources/detailed_building.glb");
collision.BuildBVH(&complexModel);
collision.SetUseBVH(true);  // Enable BVH mode

TraceLog(LOG_INFO, "BVH built with %zu triangles", collision.GetTriangleCount());
```

#### BVH with Transformation
```cpp
// Build BVH with model transformation
Matrix transform = MatrixMultiply(
    MatrixRotateY(DEG2RAD * 45), 
    MatrixTranslate(10.0f, 0.0f, 5.0f)
);
collision.BuildBVH(&complexModel, transform);
```

### BVH Operations

#### Precise Collision Detection
```cpp
Collision objectA, objectB;

// Build BVH for both objects
objectA.BuildBVH(&modelA);
objectB.BuildBVH(&modelB);
objectA.SetUseBVH(true);
objectB.SetUseBVH(true);

// Check precise collision
if (objectA.IntersectsBVH(objectB)) {
    TraceLog(LOG_WARNING, "Precise collision detected!");
    // Handle collision with triangle-level precision
}
```

#### Ray Casting
```cpp
// Create ray (e.g., for shooting or mouse picking)
Vector3 rayOrigin = camera.position;
Vector3 rayDirection = Vector3Normalize(Vector3Subtract(targetPosition, rayOrigin));
CollisionRay shootingRay(rayOrigin, rayDirection);

// Perform ray casting
float hitDistance;
Vector3 hitPoint, hitNormal;

if (collision.RaycastBVH(shootingRay, hitDistance, hitPoint, hitNormal)) {
    TraceLog(LOG_INFO, "Ray hit at distance: %.2f", hitDistance);
    TraceLog(LOG_INFO, "Hit point: (%.2f, %.2f, %.2f)", hitPoint.x, hitPoint.y, hitPoint.z);
    TraceLog(LOG_INFO, "Hit normal: (%.2f, %.2f, %.2f)", hitNormal.x, hitNormal.y, hitNormal.z);
    
    // Use hit information for gameplay (damage, effects, etc.)
    CreateExplosion(hitPoint);
    ApplyDecal(hitPoint, hitNormal);
}
```

#### Mouse Picking with BVH
```cpp
// In game loop
if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    Vector2 mousePos = GetMousePosition();
    Ray raylibRay = GetMouseRay(mousePos, camera);
    CollisionRay mouseRay(raylibRay.position, raylibRay.direction);
    
    float closestDistance = FLT_MAX;
    Collision* hitObject = nullptr;
    Vector3 hitPoint, hitNormal;
    
    // Test all BVH objects
    for (auto& object : bvhObjects) {
        float distance;
        Vector3 point, normal;
        
        if (object.collision.RaycastBVH(mouseRay, distance, point, normal)) {
            if (distance < closestDistance) {
                closestDistance = distance;
                hitObject = &object.collision;
                hitPoint = point;
                hitNormal = normal;
            }
        }
    }
    
    if (hitObject) {
        TraceLog(LOG_INFO, "Selected object at: (%.2f, %.2f, %.2f)", 
                 hitPoint.x, hitPoint.y, hitPoint.z);
        // Handle object selection
    }
}
```

#### Point Containment Testing
```cpp
Vector3 testPoint = playerPosition;

if (collision.ContainsBVH(testPoint)) {
    TraceLog(LOG_INFO, "Player is inside the building");
    // Apply indoor effects, change lighting, etc.
} else {
    TraceLog(LOG_INFO, "Player is outside");
    // Apply outdoor effects
}
```

### BVH vs AABB Comparison

#### Performance Comparison
```cpp
// Measure performance difference
Collision aabbCollision, bvhCollision;

// Setup AABB
aabbCollision.CalculateFromModel(&model);
aabbCollision.SetUseBVH(false);

// Setup BVH
bvhCollision.BuildBVH(&model);
bvhCollision.SetUseBVH(true);

// Performance test
auto start = std::chrono::high_resolution_clock::now();

// Test AABB collision (fast, less precise)
bool aabbResult = aabbCollision.Intersects(otherAABB);

auto mid = std::chrono::high_resolution_clock::now();

// Test BVH collision (slower, more precise)
bool bvhResult = bvhCollision.IntersectsBVH(otherBVH);

auto end = std::chrono::high_resolution_clock::now();

auto aabbTime = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
auto bvhTime = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

TraceLog(LOG_INFO, "AABB: %s (%lld μs), BVH: %s (%lld μs)", 
         aabbResult ? "collision" : "no collision", aabbTime.count(),
         bvhResult ? "collision" : "no collision", bvhTime.count());
```

### Game Integration Example

```cpp
class GameObject {
private:
    Model model;
    Collision collision;
    Vector3 position;
    bool usePreciseCollision = false;
    
public:
    void Initialize(const char* modelPath, bool precise = false) {
        model = LoadModel(modelPath);
        usePreciseCollision = precise;
        
        if (usePreciseCollision) {
            collision.BuildBVH(&model);
            collision.SetUseBVH(true);
            TraceLog(LOG_INFO, "Object initialized with BVH (%zu triangles)", 
                     collision.GetTriangleCount());
        } else {
            collision.CalculateFromModel(&model);
            collision.SetUseBVH(false);
            TraceLog(LOG_INFO, "Object initialized with AABB");
        }
    }
    
    bool CheckCollision(const GameObject& other) const {
        if (usePreciseCollision && other.usePreciseCollision) {
            return collision.IntersectsBVH(other.collision);
        } else {
            return collision.Intersects(other.collision);
        }
    }
    
    bool Raycast(const CollisionRay& ray, float& distance, Vector3& point, Vector3& normal) const {
        if (usePreciseCollision) {
            return collision.RaycastBVH(ray, distance, point, normal);
        }
        return false; // AABB doesn't support ray casting
    }
};
```

## When to Use BVH vs AABB

### Use BVH when:
- **Complex Geometry**: Detailed meshes with many triangles
- **Precise Collision**: Need triangle-level accuracy
- **Ray Casting**: Shooting, picking, line-of-sight
- **Realistic Physics**: Physical simulations requiring accuracy
- **Visual Quality**: Important for gameplay or visuals

### Use AABB when:
- **Simple Geometry**: Basic shapes, boxes, spheres
- **Performance Critical**: Need maximum speed
- **Approximate Collision**: Rough collision detection is sufficient
- **Many Objects**: Large numbers of simple objects
- **Broad Phase**: Initial collision filtering

### Hybrid Approach
```cpp
// Use both systems for optimal performance
class AdvancedCollision {
    Collision aabb;    // Fast broad-phase
    Collision bvh;     // Precise narrow-phase
    
    bool CheckCollision(const AdvancedCollision& other) {
        // First check AABB (fast)
        if (!aabb.Intersects(other.aabb)) {
            return false; // No collision possible
        }
        
        // Then check BVH (precise)
        return bvh.IntersectsBVH(other.bvh);
    }
};
```

## Performance Guidelines

### BVH Construction
- Build BVH once per model or when geometry changes
- Avoid rebuilding BVH every frame
- Consider caching BVH structures
- Use transformations instead of rebuilding when possible

### Runtime Performance
- BVH collision: O(log n) where n is triangle count
- AABB collision: O(1)
- Ray casting: O(log n) with early termination
- Use level-of-detail (LOD) for distant objects

### Memory Usage
```cpp
// Monitor BVH memory usage
size_t triangleCount = collision.GetTriangleCount();
size_t estimatedMemory = triangleCount * (sizeof(Triangle) + sizeof(BVHNode) / 4);
TraceLog(LOG_INFO, "BVH estimated memory: %zu bytes", estimatedMemory);
```

## Debugging BVH

```cpp
// Enable detailed logging
collision.BuildBVH(&model);
TraceLog(LOG_INFO, "BVH Statistics:");
TraceLog(LOG_INFO, "- Triangles: %zu", collision.GetTriangleCount());
TraceLog(LOG_INFO, "- Using BVH: %s", collision.IsUsingBVH() ? "yes" : "no");

// Visualize bounding boxes (add to your debug renderer)
Vector3 min = collision.GetMin();
Vector3 max = collision.GetMax();
DrawBoundingBox((BoundingBox){min, max}, GREEN);

// Test ray casting interactively
if (IsKeyPressed(KEY_SPACE)) {
    CollisionRay testRay(camera.position, Vector3Subtract(camera.target, camera.position));
    float dist;
    Vector3 point, normal;
    if (collision.RaycastBVH(testRay, dist, point, normal)) {
        // Draw hit point
        DrawSphere(point, 0.1f, RED);
        DrawLine3D(point, Vector3Add(point, Vector3Scale(normal, 0.5f)), BLUE);
    }
}
```