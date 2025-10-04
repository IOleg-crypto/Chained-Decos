# Chained Decos Developer Guide

## Project Overview

Chained Decos is a 3D parkour game built with modern C++20 and the Raylib framework. This guide provides comprehensive information for developers who want to understand, extend, or contribute to the project.

## Architecture Overview

### System Architecture

The project follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────┐
│           Application Layer         │
│   ┌─────────────┐ ┌─────────────┐   │
│   │   Game      │ │ Map Editor  │   │
│   └─────────────┘ └─────────────┘   │
└─────────────────┬───────────────────┘
                  │
┌─────────────────▼───────────────────┐
│           Engine Layer              │
│   ┌───────┐ ┌───────┐ ┌─────────┐   │
│   │Physics│ │Render │ │Collision│   │
│   └───────┘ └───────┘ └─────────┘   │
└─────────────────────────────────────┘
```

### Key Design Patterns

- **Component-Based Architecture**: Entities composed of reusable components
- **Factory Pattern**: Consistent object creation through factories
- **Observer Pattern**: Event-driven communication between systems
- **Service Locator**: Centralized access to core systems

## Development Setup

### Prerequisites

#### Required Tools
- **CMake 3.20+**: Build system configuration
- **C++20 Compiler**: GCC 10+, Clang 11+, or MSVC 19.28+
- **Git**: Version control and dependency management

#### Development Environment
- **Visual Studio 2022** (Windows) - Excellent CMake integration
- **CLion** (Cross-platform) - Superior CMake workflow
- **VS Code** - Lightweight with CMake Tools extension

### Building from Source

#### Initial Setup

```bash
# Clone repository with submodules
git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos

# Configure development build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_INFO=ON -DBUILD_TESTS=ON
cmake --build . --config Debug
```

#### Development Build Options

```bash
# Full development configuration
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DENABLE_DEBUG_INFO=ON \
         -DENABLE_WARNINGS=ON \
         -DENABLE_SANITIZERS=ON \
         -DBUILD_TESTS=ON \
         -DBUILD_MAP_EDITOR=ON \
         -DENABLE_DEV_TOOLS=ON

# Fast iteration build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DFAST_CONFIG=ON \
         -DENABLE_UNITY_BUILD=ON
```

## Code Organization

### Directory Structure

```
ChainedDecos/
├── src/
│   ├── Engine/           # Core engine systems
│   │   ├── Collision/    # Physics and collision detection
│   │   ├── Physics/      # Physics simulation
│   │   ├── Render/       # Rendering pipeline
│   │   ├── Model/        # 3D model management
│   │   ├── Math/         # Mathematical utilities
│   │   └── Kernel/       # Service management
│   ├── Game/             # Game-specific logic
│   │   ├── Player/       # Player mechanics
│   │   ├── Map/          # Map generation and loading
│   │   ├── Menu/         # UI and menus
│   │   └── MapEditor/    # Level editor
│   └── main.cpp          # Application entry point
├── resources/            # Game assets
├── tests/                # Unit and integration tests
├── benchmarks/           # Performance benchmarks
└── docs/                 # Documentation
```

### Engine Components

#### Collision System

The collision system uses a hybrid approach:

- **BVH (Bounding Volume Hierarchy)**: For complex mesh collisions
- **AABB (Axis-Aligned Bounding Boxes)**: For simple geometry and performance

```cpp
// Example: Setting up collision for a platform
auto platform = std::make_shared<GameObject>();
auto collision = std::make_shared<CollisionComponent>();

// Use AABB for simple rectangular platforms
BoundingBox box = {
    .min = Vector3{-5, -1, -5},
    .max = Vector3{ 5,  1,  5}
};
collision->SetBoundingBox(box);

// Use BVH for complex mesh collisions
if (complexGeometry) {
    collision->SetMeshCollision(mesh);
    collision->BuildBVH();
}

platform->AddComponent(collision);
```

#### Physics System

Physics simulation with realistic movement:

```cpp
// Example: Player physics setup
auto physics = std::make_shared<PhysicsComponent>(collision);

physics->SetGravity(-9.81f);
physics->SetJumpForce(5.0f);
physics->SetMoveSpeed(8.0f);
physics->SetAirControl(0.1f);

player->AddComponent(physics);
```

#### Rendering Pipeline

Efficient 3D rendering with debug capabilities:

```cpp
// Example: Render manager setup
auto renderManager = std::make_shared<RenderManager>();

// Configure rendering
renderManager->SetCamera(playerCamera);
renderManager->SetShadowMapSize(2048);
renderManager->EnableHDR(true);

// Debug visualization
renderManager->EnableDebugMode(DebugMode::Collision);
renderManager->EnableDebugMode(DebugMode::BVH);
```

## Adding New Features

### Creating a New Game Mode

1. **Define Difficulty Configuration**

```cpp
struct CustomDifficultyConfig {
    int platformCount = 20;
    float averageGap = 3.0f;
    float heightVariation = 5.0f;
    bool includeMovingPlatforms = true;
    Color themeColor = RED;
};
```

2. **Implement Platform Generator**

```cpp
class CustomMapGenerator : public BaseMapGenerator {
public:
    void GeneratePlatforms() override {
        for (int i = 0; i < config.platformCount; i++) {
            Vector3 position = GeneratePlatformPosition(i);
            Vector3 size = GeneratePlatformSize(i);
            Color color = GetThemeColor(i);

            AddPlatform(position, size, color);
        }
    }

    void SetupSpecialObjects() override {
        // Add moving platforms, breakables, etc.
        AddMovingPlatform(Vector3{0, 5, 0}, Vector3{10, 5, 0}, 2.0f);
    }

private:
    CustomDifficultyConfig config;
};
```

3. **Register with Game System**

```cpp
// In Game::Initialize()
auto customMode = std::make_shared<CustomGameMode>();
customMode->SetGenerator(std::make_shared<CustomMapGenerator>());
gameModes[GameMode::Custom] = customMode;
```

### Adding New Components

1. **Create Component Class**

```cpp
class HealthComponent : public IComponent {
public:
    HealthComponent(int maxHealth) : maxHealth(maxHealth), currentHealth(maxHealth) {}

    void Update(float deltaTime) override {
        // Health regeneration, damage over time, etc.
    }

    void Render() override {
        // Health bar rendering
    }

    void TakeDamage(int damage) {
        currentHealth = std::max(0, currentHealth - damage);
        if (currentHealth == 0) {
            // Handle death
        }
    }

    bool IsAlive() const { return currentHealth > 0; }
    float GetHealthPercentage() const { return (float)currentHealth / maxHealth; }

private:
    int maxHealth;
    int currentHealth;
};
```

2. **Register Component Factory**

```cpp
// In ComponentFactory::RegisterComponents()
RegisterFactory<HealthComponent>([](const nlohmann::json& config) {
    return std::make_shared<HealthComponent>(config["maxHealth"]);
});
```

### Creating Custom Maps

#### Map File Format

Maps are stored in JSON format with the following structure:

```json
{
  "metadata": {
    "name": "Custom Parkour Course",
    "author": "Developer",
    "difficulty": "Medium",
    "description": "A challenging custom course"
  },
  "platforms": [
    {
      "type": "static",
      "position": [0, 0, 0],
      "size": [2, 0.5, 2],
      "color": [255, 255, 255, 255],
      "material": "concrete"
    },
    {
      "type": "moving",
      "startPosition": [0, 5, 0],
      "endPosition": [10, 5, 0],
      "speed": 2.0,
      "size": [3, 0.5, 3],
      "color": [255, 100, 100, 255]
    }
  ],
  "checkpoints": [
    {
      "position": [0, 1, 0],
      "size": [1, 2, 1]
    }
  ],
  "deathZones": [
    {
      "position": [0, -10, 0],
      "size": [50, 1, 50]
    }
  ]
}
```

#### Loading Custom Maps

```cpp
// Example: Loading a custom map
auto mapLoader = std::make_shared<MapLoader>();
auto map = mapLoader->LoadMap("custom_course.json");

if (map) {
    game->LoadMap(map);
    game->SetSpawnPoint(map->GetSpawnPoint());
}
```

## Testing

### Unit Testing

#### Writing Tests

```cpp
class PhysicsComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        collision = std::make_shared<CollisionComponent>();
        physics = std::make_shared<PhysicsComponent>(collision);
    }

    std::shared_ptr<CollisionComponent> collision;
    std::shared_ptr<PhysicsComponent> physics;
};

TEST_F(PhysicsComponentTest, JumpAppliesCorrectForce) {
    physics->SetGrounded(true);
    physics->Jump();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GT(velocity.y, 0.0f);
}

TEST_F(PhysicsComponentTest, GravityAffectsVelocity) {
    Vector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f); // Small time step

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_LT(newVelocity.y, initialVelocity.y);
}
```

#### Running Tests

```bash
# Run all tests
ctest

# Run specific test suite
ctest -R Physics

# Run with verbose output
ctest -V

# Run performance tests
ctest -R Benchmark
```

### Integration Testing

#### Game Loop Testing

```cpp
TEST(GameIntegrationTest, PlayerCompletesLevel) {
    Game game;
    game.Initialize();

    // Simulate player input
    InputState input;
    input.jump = true;
    input.moveForward = true;

    // Run game loop for several frames
    for (int i = 0; i < 300; i++) {
        game.HandleInput(input);
        game.Update(1.0f/60.0f);
    }

    // Verify player reached end
    Player& player = game.GetPlayer();
    EXPECT_TRUE(player.HasCompletedLevel());
}
```

## Performance Optimization

### Profiling

#### Built-in Profiler

```cpp
// Enable profiling
Profiler::GetInstance().Enable();

// Profile a code section
{
    ProfileScope scope("PlatformGeneration");

    // Code to profile
    GeneratePlatforms();
}

// Results automatically logged
```

#### Manual Profiling Points

```cpp
// Add profiling points to critical sections
void PhysicsComponent::Update(float deltaTime) {
    ProfileScope scope("PhysicsUpdate");

    UpdateVelocity(deltaTime);
    UpdatePosition(deltaTime);
    CheckCollisions();
}
```

### Optimization Techniques

#### Memory Optimization

```cpp
// Use object pooling for frequently created objects
class PlatformPool {
public:
    std::shared_ptr<Platform> Acquire() {
        if (pool.empty()) {
            return std::make_shared<Platform>();
        }
        auto platform = pool.back();
        pool.pop_back();
        return platform;
    }

    void Release(std::shared_ptr<Platform> platform) {
        platform->Reset();
        pool.push_back(platform);
    }

private:
    std::vector<std::shared_ptr<Platform>> pool;
};
```

#### CPU Optimization

```cpp
// Use spatial partitioning for collision detection
class SpatialGrid {
public:
    void AddObject(std::shared_ptr<GameObject> obj) {
        Vector3 pos = obj->GetPosition();
        GridCell& cell = GetCell(pos);
        cell.objects.push_back(obj);
    }

    std::vector<std::shared_ptr<GameObject>>
    GetNearbyObjects(const Vector3& position) {
        GridCell& cell = GetCell(position);
        return cell.objects;
    }
};
```

## Debugging

### Debug Visualization

#### Collision Debug

```cpp
// Enable collision visualization
renderManager->SetDebugMode(DebugMode::Collision);

// Draw collision shapes
for (const auto& obj : gameObjects) {
    if (auto collision = obj->GetComponent<CollisionComponent>()) {
        DrawCollisionShape(collision->GetShape());
    }
}
```

#### Performance Debug

```cpp
// Display performance metrics
void RenderPerformanceOverlay() {
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
    DrawText(TextFormat("Draw Calls: %d", renderManager->GetDrawCallCount()), 10, 40, 20, GREEN);
    DrawText(TextFormat("Physics Objects: %d", physicsManager->GetObjectCount()), 10, 70, 20, GREEN);
}
```

### Console Commands

#### Development Commands

```cpp
// Teleport to position
console->RegisterCommand("teleport", [](const std::vector<std::string>& args) {
    if (args.size() >= 3) {
        Vector3 pos = {std::stof(args[0]), std::stof(args[1]), std::stof(args[2])};
        player->SetPosition(pos);
    }
});

// Spawn test objects
console->RegisterCommand("spawn", [](const std::vector<std::string>& args) {
    if (args.size() >= 4) {
        std::string type = args[0];
        Vector3 pos = {std::stof(args[1]), std::stof(args[2]), std::stof(args[3])};
        SpawnObject(type, pos);
    }
});
```

## Asset Management

### Adding New Models

1. **Add to Resources**

```bash
# Place model files in resources/ directory
cp new_model.glb resources/
```

2. **Update Model Configuration**

```json
// In resources/models.json
{
  "models": {
    "new_model": {
      "file": "new_model.glb",
      "scale": [1.0, 1.0, 1.0],
      "collision_type": "bvh",
      "materials": {
        "default": "concrete"
      }
    }
  }
}
```

3. **Load in Code**

```cpp
auto model = resourceManager->LoadModel("new_model");
auto gameObject = std::make_shared<GameObject>(model);
```

### Creating Materials

#### Material Definition

```json
{
  "materials": {
    "metal": {
      "base_color": [0.8, 0.8, 0.9],
      "metallic": 1.0,
      "roughness": 0.2,
      "emissive": [0.0, 0.0, 0.0]
    },
    "concrete": {
      "base_color": [0.5, 0.5, 0.5],
      "metallic": 0.0,
      "roughness": 0.8,
      "emissive": [0.0, 0.0, 0.0]
    }
  }
}
```

## Deployment

### Building for Production

```bash
# Production build configuration
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_OPTIMIZATIONS=ON \
         -DENABLE_PROFILING=OFF \
         -DENABLE_DEBUG_INFO=OFF \
         -DBUILD_TESTS=OFF

# Create release package
cmake --build . --config Release --target package
```

### Platform-Specific Builds

#### Windows

```bash
# MSVC Release build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# MinGW-w64 build
cmake -G "MinGW Makefiles" .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

#### Linux

```bash
# GCC Release build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build . --config Release

# Clang Release build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build . --config Release
```

#### macOS

```bash
# macOS Release build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
cmake --build . --config Release
```

## Contributing

### Code Style Guidelines

#### C++ Best Practices
- Use C++20 features appropriately
- Follow RAII (Resource Acquisition Is Initialization)
- Use smart pointers for resource management
- Avoid raw pointers except where necessary

#### Naming Conventions
- **Classes**: PascalCase (`PhysicsComponent`)
- **Functions**: camelCase (`updatePhysics()`)
- **Variables**: camelCase (`playerVelocity`)
- **Constants**: SCREAMING_SNAKE_CASE (`MAX_JUMP_HEIGHT`)

#### Code Organization
- One class per header file
- Implementation in corresponding .cpp file
- Group related functions together
- Use namespaces to avoid conflicts

### Submitting Changes

1. **Create Feature Branch**
```bash
git checkout -b feature/amazing-feature
```

2. **Make Changes**
- Follow coding standards
- Add tests for new functionality
- Update documentation

3. **Test Thoroughly**
```bash
# Run full test suite
ctest

# Test performance impact
cmake --build . --target benchmarks
```

4. **Submit Pull Request**
- Provide clear description
- Reference related issues
- Include testing information

## Troubleshooting

### Common Development Issues

#### Build Problems

**CMake Configuration Issues:**
```bash
# Clear build directory and reconfigure
rm -rf build/
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

**Dependency Issues:**
```bash
# Clear dependency cache
rm -rf build/.deps/
cmake ..  # Will refetch dependencies
```

#### Runtime Issues

**Memory Leaks:**
- Use sanitizers in development builds
- Check for missing smart pointer usage
- Verify proper cleanup in destructors

**Performance Issues:**
- Profile with built-in profiler
- Check for unnecessary allocations
- Optimize collision detection complexity

#### Collision Problems

**Objects Falling Through Floor:**
- Verify collision mesh integrity
- Check BVH tree construction
- Validate collision response code

**Incorrect Collision Detection:**
- Visualize collision shapes with debug mode
- Check collision mask/layer settings
- Verify collision geometry accuracy

## Advanced Topics

### Custom Shaders

#### Creating Shader Programs

```glsl
// Vertex shader
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

```glsl
// Fragment shader
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;

void main() {
    FragColor = vec4(objectColor, 1.0);
}
```

### Network Multiplayer (Future)

#### Network Architecture Planning

- **Client-Server Model**: Authoritative server for game state
- **UDP Protocol**: Low latency for real-time gameplay
- **State Synchronization**: Efficient state updates
- **Prediction**: Client-side prediction for smooth movement

### Advanced Physics

#### Soft Body Physics (Future)

- **Mass-Spring Systems**: For deformable objects
- **Particle Systems**: For fluid and cloth simulation
- **Constraint Solving**: For realistic interactions

## Getting Help

### Resources

- **API Documentation**: `docs/API.md`
- **User Guide**: `docs/UserGuide.md`
- **Code Examples**: Look for `Examples/` directory
- **Community**: GitHub Discussions for questions

### Support Channels

1. **Documentation**: Check guides first
2. **Code Search**: Use existing patterns
3. **Issue Tracker**: Report bugs and request features
4. **Discussions**: Ask questions and share knowledge

---

This developer guide provides a comprehensive foundation for working with the Chained Decos codebase. As the project evolves, this guide will be updated to reflect new features and best practices.