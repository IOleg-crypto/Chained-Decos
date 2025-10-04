# Chained Decos API Documentation

## Overview

This document provides comprehensive API documentation for the Chained Decos game engine and game systems.

## Engine Architecture

### Core Systems

#### PhysicsComponent

The `PhysicsComponent` class handles all physics-related calculations for game objects.

```cpp
class PhysicsComponent {
public:
    // Constructor
    PhysicsComponent(std::shared_ptr<CollisionComponent> collision);

    // Core physics methods
    void Update(float deltaTime);
    void ApplyForce(const Vector3& force);
    void SetVelocity(const Vector3& velocity);
    Vector3 GetVelocity() const;

    // Ground and collision state
    bool IsGrounded() const;
    void SetGrounded(bool grounded);

    // Movement parameters
    void SetGravity(float gravity);
    void SetJumpForce(float jumpForce);
    void SetMoveSpeed(float speed);

private:
    Vector3 velocity;
    Vector3 acceleration;
    bool isGrounded;
    float gravity;
    float jumpForce;
    float moveSpeed;
};
```

#### CollisionSystem

Manages collision detection using Bounding Volume Hierarchy (BVH) for complex meshes.

```cpp
class CollisionSystem {
public:
    // Collision detection
    bool CheckCollision(const CollisionComponent& a, const CollisionComponent& b);
    std::vector<CollisionPair> GetCollisionPairs();

    // BVH management
    void BuildBVH(const std::vector<CollisionComponent>& objects);
    void UpdateBVH();
    void TraverseBVH(const BoundingBox& queryBox);

private:
    BVHTree bvhTree;
    std::vector<CollisionPair> collisionPairs;
};
```

#### RenderManager

Handles all rendering operations and debug visualization.

```cpp
class RenderManager {
public:
    // Rendering pipeline
    void BeginFrame();
    void RenderObject(const ModelComponent& model, const Transform& transform);
    void RenderUI();
    void EndFrame();

    // Debug visualization
    void DrawCollisionBoxes(const std::vector<CollisionComponent>& objects);
    void DrawBVH(const BVHTree& bvh);
    void DrawPerformanceMetrics();

    // Camera management
    void SetCamera(const Camera3D& camera);
    Camera3D GetCamera() const;
};
```

### Component System

#### Base Component Interface

```cpp
class IComponent {
public:
    virtual ~IComponent() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual const std::string& GetName() const = 0;
};
```

#### CollisionComponent

```cpp
class CollisionComponent : public IComponent {
public:
    // Collision shapes
    void SetBoundingBox(const BoundingBox& box);
    void SetMeshCollision(const Mesh& mesh);
    void SetSphereCollision(float radius);

    // Collision queries
    bool Intersects(const CollisionComponent& other) const;
    std::vector<Vector3> GetCollisionPoints() const;

    // Properties
    CollisionType GetType() const;
    void SetTrigger(bool isTrigger);

private:
    CollisionType type;
    BoundingBox boundingBox;
    std::shared_ptr<Mesh> collisionMesh;
    bool isTrigger;
};
```

## Game Systems

### Player System

#### Player Class

```cpp
class Player {
public:
    // Lifecycle
    Player();
    ~Player();
    void Initialize();
    void Update(float deltaTime);
    void Render();

    // Input handling
    void HandleInput(const InputState& input);
    void SetInputMapping(const std::string& action, KeyBinding binding);

    // Movement
    void Move(const Vector3& direction);
    void Jump();
    void Sprint(bool enable);

    // State queries
    Vector3 GetPosition() const;
    Vector3 GetVelocity() const;
    bool IsGrounded() const;
    PlayerState GetState() const;

private:
    std::shared_ptr<PhysicsComponent> physics;
    std::shared_ptr<CollisionComponent> collision;
    std::shared_ptr<ModelComponent> model;
    std::shared_ptr<InputComponent> input;

    PlayerState currentState;
    std::map<std::string, KeyBinding> inputMappings;
};
```

### Map System

#### ParkourMapGenerator

```cpp
class ParkourMapGenerator {
public:
    // Map generation
    void GenerateMap(DifficultyLevel difficulty);
    void GeneratePlatforms(int count, const PlatformConfig& config);
    void GenerateObstacles();

    // Platform management
    void AddPlatform(const Vector3& position, const Vector3& size, const Color& color);
    void AddMovingPlatform(const Vector3& start, const Vector3& end, float speed);
    void AddBreakablePlatform(const Vector3& position, int health);

    // Collision setup
    void SetupPlatformCollisions();
    void SetupDeathZones();
    void SetupCheckpoints();

    // Map queries
    std::vector<Platform> GetPlatforms() const;
    std::vector<Checkpoint> GetCheckpoints() const;
    BoundingBox GetMapBounds() const;

private:
    std::vector<Platform> platforms;
    std::vector<Checkpoint> checkpoints;
    std::vector<DeathZone> deathZones;
    DifficultyLevel currentDifficulty;
};
```

### Menu System

#### MenuManager

```cpp
class MenuManager {
public:
    // Menu navigation
    void ShowMainMenu();
    void ShowPauseMenu();
    void ShowSettingsMenu();
    void ShowMapSelection();

    // Menu state
    void PushMenu(const std::string& menuName);
    void PopMenu();
    std::string GetCurrentMenu() const;

    // Event handling
    void HandleInput(const InputEvent& event);
    void Update(float deltaTime);
    void Render();

private:
    std::stack<std::string> menuStack;
    std::map<std::string, std::unique_ptr<Menu>> menus;
    InputManager& inputManager;
};
```

## Utility Systems

### Configuration Manager

```cpp
class ConfigManager {
public:
    // Configuration loading
    void LoadConfig(const std::string& filename);
    void SaveConfig(const std::string& filename);

    // Value access
    template<typename T>
    T GetValue(const std::string& key, const T& defaultValue = T{});

    template<typename T>
    void SetValue(const std::string& key, const T& value);

    // Sections
    void SetSection(const std::string& section);
    std::vector<std::string> GetSections() const;

private:
    nlohmann::json configData;
    std::string currentSection;
};
```

### Resource Manager

```cpp
class ResourceManager {
public:
    // Resource loading
    std::shared_ptr<Model> LoadModel(const std::string& filename);
    std::shared_ptr<Texture> LoadTexture(const std::string& filename);
    std::shared_ptr<Font> LoadFont(const std::string& filename);

    // Resource management
    void UnloadResource(const std::string& name);
    void UnloadAll();
    bool IsLoaded(const std::string& name) const;

    // Resource information
    size_t GetMemoryUsage() const;
    std::vector<std::string> GetLoadedResources() const;

private:
    std::map<std::string, std::shared_ptr<void>> resources;
    std::map<std::string, ResourceType> resourceTypes;
};
```

## Event System

### Event Types

```cpp
// Core events
struct CollisionEvent {
    CollisionComponent* objectA;
    CollisionComponent* objectB;
    Vector3 contactPoint;
    Vector3 normal;
};

struct InputEvent {
    InputType type;
    KeyCode key;
    Vector2 mousePosition;
    bool pressed;
};

struct GameStateEvent {
    GameState oldState;
    GameState newState;
    float transitionTime;
};
```

### Event Manager

```cpp
class EventManager {
public:
    // Event subscription
    template<typename EventType>
    void Subscribe(const std::string& eventName, std::function<void(const EventType&)> callback);

    template<typename EventType>
    void Unsubscribe(const std::string& eventName);

    // Event dispatching
    template<typename EventType>
    void Dispatch(const std::string& eventName, const EventType& event);

    // Event processing
    void ProcessEvents();
    void ClearEvents();

private:
    std::map<std::string, std::vector<std::function<void(const void*)>>> subscribers;
    std::queue<std::pair<std::string, std::shared_ptr<void>>> eventQueue;
};
```

## Extension Guide

### Creating Custom Components

1. Inherit from `IComponent`
2. Implement required virtual methods
3. Register with the component system

```cpp
class CustomComponent : public IComponent {
public:
    void Update(float deltaTime) override;
    void Render() override;
    const std::string& GetName() const override { return name; }

private:
    std::string name = "CustomComponent";
};
```

### Adding New Game Modes

1. Create difficulty configuration
2. Implement platform generation logic
3. Add scoring and win conditions

```cpp
class CustomDifficulty : public BaseDifficulty {
public:
    void GeneratePlatforms() override;
    int CalculateScore() override;
    bool CheckWinCondition() override;
};
```

## Best Practices

### Memory Management
- Use smart pointers for resource management
- Avoid circular dependencies
- Implement proper cleanup in destructors

### Performance
- Use BVH for complex collision detection
- Implement object pooling for frequently created objects
- Use spatial partitioning for large worlds

### Code Organization
- Follow single responsibility principle
- Use dependency injection for testability
- Maintain consistent naming conventions

## Testing

### Unit Testing

```cpp
class PhysicsComponentTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<PhysicsComponent> physics;
    std::shared_ptr<CollisionComponent> collision;
};
```

### Integration Testing

```cpp
TEST(GameIntegrationTest, PlayerMovement) {
    Game game;
    game.Initialize();

    Player& player = game.GetPlayer();
    player.Move(Vector3{1, 0, 0});

    ASSERT_EQ(player.GetPosition().x, 1.0f);
}
```

## Debugging

### Debug Visualization

```cpp
// Enable collision debug
game.GetRenderManager().SetDebugMode(DebugMode::Collision);

// Enable performance metrics
game.GetRenderManager().SetDebugMode(DebugMode::Performance);

// Enable BVH visualization
game.GetRenderManager().SetDebugMode(DebugMode::BVH);
```

### Console Commands

```cpp
// Show FPS
console.Execute("fps");

// Set resolution
console.Execute("res 1920x1080");

// Toggle noclip (planned)
console.Execute("noclip");
```

## Contributing

When extending the API:

1. Update this documentation
2. Add unit tests for new functionality
3. Follow existing code patterns
4. Update build configuration if needed

## Version History

- **v1.0.0**: Initial API documentation
- **v0.9.0**: Enhanced physics and collision systems
- **v0.8.0**: Map editor integration
- **v0.7.0**: Component system refactor