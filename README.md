# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![Build Status](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/build.yml)](https://github.com/IOleg-crypto/Chained-Decos/actions) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

A fast-paced 3D parkour game built with modern C++20 and [raylib](https://www.raylib.com/). Features a custom physics engine with BVH collision detection, modular architecture, and comprehensive debugging tools.

![Game Screenshot](https://i.imgur.com/d9Bxmsq.jpeg) <!-- TODO: Add actual screenshot -->

> [!IMPORTANT]
> This project is under active development. Expect frequent changes and engine refactors.

## 🎮 Overview

Chained Decos is a momentum-based parkour game where players navigate through procedurally generated courses using fluid movement mechanics. The game combines precise physics simulation with intuitive controls to create challenging yet accessible gameplay.

### Key Highlights
- **Momentum-Based Physics**: Realistic movement with gravity, drag, and collision response
- **Procedural Generation**: Dynamic parkour courses with varying difficulty levels
- **Modular Architecture**: Clean separation between engine, game logic, and tools
- **Developer Tools**: Integrated map editor, debugging overlays, and performance monitoring

## 🏗️ Architecture & Modules

Chained Decos follows a modular architecture with clear separation of concerns:

### Core Engine (`src/Engine/`)
The foundation layer providing essential services:

#### [`Engine/`](src/Engine/) - Main Engine Core
- **Purpose**: Window management, rendering pipeline, and core services
- **Key Components**: `Engine` class, render manager, input manager
- **Features**: Cross-platform window handling, debug overlays, performance monitoring

#### [`Engine/Physics/`](src/Engine/Physics/) - Physics Simulation
- **Purpose**: Realistic movement and collision response
- **Key Classes**: `PhysicsComponent`, `SurfaceComponent`
- **Features**: Gravity simulation, velocity management, kinematic objects

#### [`Engine/Collision/`](src/Engine/Collision/) - Collision Detection
- **Purpose**: Leverages raylib's built-in AABB collision detection, enhanced with BVH optimization
- **Key Classes**: `CollisionSystem`, `CollisionManager`, `BVHNode`
- **Features**: Optimized broad-phase, triangle-level narrow-phase, collision pooling

#### [`Engine/Model/`](src/Engine/Model/) - 3D Model Management
- **Purpose**: Efficient 3D model loading, caching, and rendering
- **Key Classes**: `ModelLoader`, `ModelCache`, `ModelInstance`
- **Features**: LRU caching, LOD support, JSON configuration, animation support

### Game Layer (`src/Game/`)
Game-specific logic and mechanics:

#### [`Game/Player/`](src/Game/Player/) - Player System
- **Purpose**: Player character with component-based architecture
- **Components**: `PlayerMovement`, `PlayerInput`, `PlayerCollision`, `PlayerModel`
- **Features**: Fluid parkour movement, camera control, collision response

#### [`Game/Map/`](src/Game/Map/) - Map Management
- **Purpose**: Loading and managing game maps
- **Key Classes**: `MapLoader`, `GameMap`
- **Features**: JSON map format, procedural generation, editor integration

#### [`Game/Menu/`](src/Game/Menu/) - User Interface
- **Purpose**: Game menus and UI management
- **Key Classes**: `Menu`, `SettingsManager`, `ConsoleManager`
- **Features**: ImGui integration, settings persistence, developer console

### Tools (`src/MapEditor/`)
Development and content creation tools:

#### [`MapEditor/`](src/MapEditor/) - Level Editor
- **Purpose**: Visual level creation and editing
- **Architecture**: Facade pattern with subsystem managers
- **Features**: Real-time preview, object placement, map export

## ✨ Features

### Core Gameplay
- **3D Parkour Mechanics**: Momentum-based movement with precise controls
- **Dynamic Map Generation**: Procedurally generated courses with multiple difficulty levels
- **Real-time Physics**: Advanced collision detection and response system
- **Multiple Game Modes**: Test, Easy, Medium, Hard, and Speedrun difficulties
- **Timer System**: Cross-platform timer display with dynamic font scaling

### Development Tools
- **Integrated Map Editor**: Full-featured level editor with real-time preview
- **Particle System** *(Planning)*: Dynamic visual effects for enhanced gameplay
- **Lighting System** *(Planning)*: Advanced lighting with multiple light sources and shadows
- **Material Editor** *(Planning)*: Comprehensive material system for visual customization

### Technical Features
- **High-Performance Engine**: Optimized rendering and physics systems
- **Cross-Platform Support**: Windows, macOS, and Linux compatibility
- **Modular Architecture**: Clean separation of engine, game logic, and tools
- **Comprehensive Testing**: Unit tests with Google Test framework
- **Advanced Debugging**: Collision visualization, performance metrics, real-time tweaking
- **Developer Console**: In-game command console with Source engine-style commands

## 🚀 Installation

### Prerequisites
- **CMake**: 3.20+
- **C++20 compiler**: GCC 10+, Clang 11+, or MSVC 2019+
- **Git**: For cloning with submodules

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
   cd Chained-Decos
   ```

2. **Configure and build**:
   ```bash
   mkdir build
   cd build
   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release  # Or Debug for development
   cmake --build . --config Release
   ```

3. **Run the game**:
   ```bash
   # Windows
   .\src\Game\ChainedDecosGame.exe

   # Linux/macOS
   ./src/Game/ChainedDecosGame
   ```

### Map Editor Build
To build the integrated map editor:
```bash
cmake .. -DBUILD_MAP_EDITOR=ON
cmake --build .
```

## 🎯 Usage

### Game Controls
| Key | Action |
|-----|--------|
| `WASD` | Move |
| `Space` | Jump |
| `Shift` | Sprint |
| `Mouse` | Look around |
| `T` | Emergency reset (teleport to spawn) |

### Debug & UI
| Key | Action |
|-----|--------|
| `F1` | Toggle main menu |
| `F2` | Toggle debug info overlay |
| `F3` | Toggle collision debug visualization |
| `ESC` | Pause/return to menu |
| `~` | Toggle developer console |

### Developer Console Commands
```bash
help          # Show available commands
clear         # Clear console output
quit/exit     # Exit game
fps           # Show current FPS
res <WxH>     # Set resolution (e.g., res 1920x1080)
fullscreen    # Toggle fullscreen mode
vsync <on/off> # Toggle VSync
```

### Command Line Arguments
```bash
ChainedDecosGame.exe -fullscreen -width 1920 -height 1080 -novsync
```

Available options:
- `-width <width>` - Set window width
- `-height <height>` - Set window height
- `-fullscreen` - Start in fullscreen mode
- `-novsync` - Disable VSync
- `-map <mapname>` - Load specific map
- `-dev` - Enable developer mode

### Configuration
Edit [`game.cfg`](game.cfg) for persistent settings:
- Resolution and display options
- Audio volume and sensitivity
- Control bindings
- Debug settings

## 🧪 Testing

Run the unit tests using Google Test:
```bash
# From build directory
./tests/gtest
```

Key test modules:
- **Physics Tests**: [`tests/engine/PhysicsComponentTest.cpp`](tests/engine/PhysicsComponentTest.cpp)
- **Collision Tests**: BVH and AABB collision accuracy
- **Map Generation Tests**: [`tests/game/ParkourMapGeneratorTest.cpp`](tests/game/ParkourMapGeneratorTest.cpp)

## 🤝 Contributing

We welcome contributions! Please follow these steps:

1. **Fork the repository**
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/amazing-feature
   ```
3. **Make your changes** and add tests
4. **Commit your changes**:
   ```bash
   git commit -m 'Add amazing feature'
   ```
5. **Push to the branch**:
   ```bash
   git push origin feature/amazing-feature
   ```
6. **Open a Pull Request**

### Development Guidelines
- Follow C++20 best practices and RAII principles
- Add unit tests for new features
- Update documentation for API changes
- Use the established code style (similar to Google C++ Style Guide)

## 📊 Performance

### System Requirements
- **Minimum**: CPU with SSE2, 4GB RAM, OpenGL 3.3+
- **Recommended**: Modern CPU, 8GB RAM, dedicated GPU

### Optimization Features
- **BVH Collision Detection**: O(log n) collision queries
- **Model Caching**: LRU cache for frequently used assets
- **Parallel Physics**: Multi-threaded physics updates
- **Memory Pooling**: Efficient collision object management

## 📝 License

This project is licensed under the MIT License - see the [`LICENSE`](LICENSE) file for details.

## 🙏 Acknowledgments

- **[raylib](https://www.raylib.com/)**: Cross-platform game development library
- **[Dear ImGui](https://github.com/ocornut/imgui)**: Bloat-free graphical user interface
- **[Google Test](https://github.com/google/googletest)**: C++ testing framework
- **Community Contributors**: Thanks to all who have contributed!

---

**Made with ❤️ using raylib, ImGui, and modern C++20**

*Chained Decos - Where momentum meets precision*
