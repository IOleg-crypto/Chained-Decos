# CHEngine\Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![Build Status](https://img.shields.io/github/actions/workflow/status/IOleg-crypto/Chained-Decos/build.yml)](https://github.com/IOleg-crypto/Chained-Decos/actions) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

A fast-paced 3D parkour game built with modern C++23 and [raylib](https://www.raylib.com/). Features a custom physics engine with BVH collision detection, modular architecture, and comprehensive debugging tools.

![Game Screenshot](https://i.imgur.com/d9Bxmsq.jpeg)

> [!IMPORTANT]
> This project is under active development. Expect frequent changes and engine refactors.

## 🎮 Overview

Chained Decos is a momentum-based parkour game where players navigate through procedurally generated courses using fluid movement mechanics. The game combines precise physics simulation with intuitive controls to create challenging yet accessible gameplay.

### Key Highlights

- **Momentum-Based Physics**: Realistic movement with gravity, drag, and collision response
- **Procedural Generation**: Dynamic parkour courses with varying difficulty levels
- **Modular Architecture**: Clean separation between engine, game logic, and tools
- **Developer Tools**: Integrated map editor, debugging overlays, and performance monitoring

## 🏗️ Architecture

> [!WARNING] > **Architecture is evolving**: This project is under active development, and the internal architecture may change frequently. The structure described below represents the current state and may not reflect future iterations. Always refer to the source code for the most up-to-date architecture.

Chained Decos follows a **modular architecture** with clear separation between engine core, game logic, and development tools:

## ✨ Features

### Core Gameplay

- **3D Parkour Mechanics**: Momentum-based movement with precise controls
- **Dynamic Map Generation**: Procedurally generated courses with multiple difficulty levels
- **Real-time Physics**: Advanced collision detection and response system
- **Multiple Game Modes**: Test, Easy, Medium, Hard, and Speedrun difficulties
- **Timer System**: Cross-platform timer display with dynamic font scaling

### Development Tools

- **Integrated Map Editor**: Full-featured level editor with real-time preview
- **Particle System** _(Planning)_: Dynamic visual effects for enhanced gameplay
- **Lighting System** _(Planning)_: Advanced lighting with multiple light sources and shadows
- **Material Editor** _(Planning)_: Comprehensive material system for visual customization

### Technical Features

- **High-Performance Engine**: Optimized rendering and physics systems
- **Cross-Platform Support**: Windows, macOS, and Linux compatibility
- **Comprehensive Testing**: Unit tests with Google Test framework
- **Advanced Debugging**: Collision visualization, performance metrics, real-time tweaking
- **Developer Console**: In-game command console with Source engine-style commands

## 🚀 Installation

### Prerequisites

#### Required

- **CMake**: 3.25 or higher
  - Download: https://cmake.org/download/
  - Verify: `cmake --version`
- **Ninja Build System**: Required for building
  - **Windows**: Included with Visual Studio or download from [GitHub Releases](https://github.com/ninja-build/ninja/releases)
  - **Linux**: `sudo apt-get install ninja-build` (Ubuntu/Debian) or `sudo pacman -S ninja` (Arch)
  - **macOS**: `brew install ninja`
  - Verify: `ninja --version`
- **C++23 compatible compiler**:
  - **Windows**: MSVC 2022 17.4+ or MinGW-w64 with GCC 13+
  - **Linux**: GCC 13+ or Clang 17+
- **Git**: For cloning the repository

#### System Dependencies

**Windows:**

- Visual Studio 2022 (for MSVC) or MinGW-w64 (for GCC)
- Windows SDK

**Linux (Ubuntu/Debian):**

```bash
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev
```

**Linux (Arch):**

```bash
sudo pacman -S base-devel cmake ninja mesa libgl libx11 \
  libxrandr libxi libxcursor libxinerama glfw
```

### Dependencies

All dependencies are automatically downloaded via CMake's `FetchContent` - no manual installation needed:

- **raylib 5.5**: Graphics and window management
- **nlohmann/json 3.12.0**: JSON parsing
- **Dear ImGui**: GUI library (included in `include/imgui`)
- **rlImGui**: Raylib-ImGui integration (included in `include/rlImGui`)
- **Native File Dialog Extended**: File dialogs (auto-fetched)
- **GoogleTest** (optional): For unit tests if `BUILD_TESTS=ON`

Dependencies are cached in `.deps/` directory for faster subsequent builds.

### Build Instructions

#### Quick Start

```bash
# 1. Clone repository
git clone https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos

# 2. Create build directory
mkdir build && cd build

# 3. Configure with Ninja
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# 4. Build
cmake --build . --config Release

# 5. Run
./bin/ChainedDecos  # Linux
.\bin\ChainedDecos.exe  # Windows
```

#### Detailed Build Steps

##### 1. Clone the Repository

```bash
git clone https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos
```

> **Note**: No submodules needed - all dependencies are auto-fetched by CMake.

##### 2. Create Build Directory

```bash
mkdir build
cd build
```

##### 3. Configure CMake with Ninja

**Release build** (recommended):

```bash
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

**Debug build** (for development):

```bash
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

**With Map Editor** (enabled by default):

```bash
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_MAP_EDITOR=ON
```

**Without tests** (faster build):

```bash
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
```

##### 4. Build the Project

```bash
# Using CMake (recommended)
cmake --build . --config Release

# Or directly with Ninja
ninja
```

##### 5. Run the Game

The executable will be located at:

- **Windows**: `build/bin/ChainedDecos.exe` (Release) or `build/bin/ChainedDecos_debug.exe` (Debug)
- **Linux/macOS**: `build/bin/ChainedDecos` (Release) or `build/bin/ChainedDecos_debug` (Debug)

```bash
# From build directory
# Windows
.\bin\ChainedDecos.exe

# Linux/macOS
./bin/ChainedDecos
```

### Platform-Specific Instructions

#### Windows

**Prerequisites:**

- CMake 3.25+
- Ninja (download from [GitHub](https://github.com/ninja-build/ninja/releases) or install via Chocolatey: `choco install ninja`)
- Visual Studio 2022 with C++ tools (MSVC) or MinGW-w64 (GCC 13+)

**Build:**

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Common Issues:**

- **"CMake 3.25 required"**: Update CMake from [cmake.org](https://cmake.org/download/)
- **"Ninja not found"**: Add Ninja to PATH or use full path: `cmake .. -G Ninja -DCMAKE_MAKE_PROGRAM="C:/path/to/ninja.exe"`
- **"C++23 not supported"**: Update to Visual Studio 2022 17.4+ or MinGW-w64 GCC 13+

#### Linux

**Ubuntu/Debian:**

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev

# Build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run
./bin/ChainedDecos
```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake ninja mesa libgl libx11 \
  libxrandr libxi libxcursor libxinerama glfw

mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Fedora:**

```bash
sudo dnf install gcc-c++ cmake ninja-build mesa-libGL-devel \
  libX11-devel libXrandr-devel libXi-devel libXcursor-devel \
  libXinerama-devel glfw-devel

mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Common Issues:**

- **OpenGL errors**: `sudo apt-get install libgl1-mesa-dev`
- **X11 errors**: `sudo apt-get install libx11-dev libxrandr-dev`
- **Wayland**: Project uses X11 by default (configured in CMakeLists.txt)

### Build

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run
./bin/ChainedDecos
```

### CMake Build Options

```bash
cmake .. -G Ninja -DOPTION_NAME=VALUE
```

| Option                 | Default   | Description                                                    |
| ---------------------- | --------- | -------------------------------------------------------------- |
| `CMAKE_BUILD_TYPE`     | `Release` | Build type: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |
| `BUILD_TESTS`          | `ON`      | Build unit tests                                               |
| `BUILD_MAP_EDITOR`     | `ON`      | Build the map editor tool                                      |
| `ENABLE_OPTIMIZATIONS` | `ON`      | Enable compiler optimizations                                  |
| `ENABLE_WARNINGS`      | `OFF`     | Enable strict compiler warnings                                |
| `ENABLE_UNITY_BUILD`   | `ON`      | Use unity build for faster compilation                         |
| `DISABLE_ALL_WARNINGS` | `ON`      | Suppress all compiler warnings                                 |
| `ENABLE_AUDIO`         | `ON`      | Enable audio support                                           |
| `ENABLE_PROFILING`     | `OFF`     | Enable profiling support                                       |
| `ENABLE_SANITIZERS`    | `OFF`     | Enable address/undefined sanitizers (Debug only)               |

**Example with multiple options:**

```bash
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF \
  -DBUILD_MAP_EDITOR=ON \
  -DENABLE_WARNINGS=ON
```

### Troubleshooting

#### Build Errors

**"CMake version 3.25 or higher required"**

```bash
# Update CMake
# Windows: Download from cmake.org
# Linux: sudo apt-get upgrade cmake
```

**"Ninja not found"**

```bash
# Install Ninja
# Windows: choco install ninja or download from GitHub
# Linux: sudo apt-get install ninja-build

# Or specify path
cmake .. -G Ninja -DCMAKE_MAKE_PROGRAM="/path/to/ninja"
```

**"C++23 standard not found"**

```bash
# Check compiler version
g++ --version  # Need GCC 13+
clang++ --version  # Need Clang 17+

# Update compiler if needed
# Ubuntu: sudo apt-get install gcc-13 g++-13
# Windows: Update Visual Studio or MinGW
```

**"raylib not found"**

- This is normal - CMake auto-fetches raylib on first configure
- Check internet connection
- Dependencies cached in `.deps/` for offline builds

**"Undefined reference" errors (Linux)**

- Usually auto-handled, but verify: `cmake .. -DENABLE_MULTITHREADING=ON`

#### Runtime Errors

**"Failed to open window"**

- Check if another instance is running
- Verify OpenGL 3.3+: `glxinfo | grep "OpenGL version"` (Linux)
- Update graphics drivers

**"Missing resources"**

- Resources auto-copied to build directory
- If missing, manually copy `resources/` to `build/bin/`

**"DLL not found" (Windows)**

- Copy required DLLs to `build/bin/`
- Or add build directory to PATH

#### Clean Build

```bash
# Remove build directory
rm -rf build  # Linux/macOS
rmdir /s build  # Windows

# Remove dependency cache (optional - will re-download)
rm -rf .deps  # Linux/macOS

# Reconfigure and build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Running Tests

```bash
# Build with tests (enabled by default)
cmake .. -G Ninja -DBUILD_TESTS=ON
cmake --build .

# Run tests
# Windows
.\bin\tests\ChainedDecosUnitTests.exe

# Linux/macOS
./bin/tests/ChainedDecosUnitTests
```

### Building Map Editor

The map editor is built by default. Executable location:

- **Windows**: `build/bin/ChainedDecosMapEditor.exe`
- **Linux/macOS**: `build/bin/ChainedDecosMapEditor`

To disable:

```bash
cmake .. -G Ninja -DBUILD_MAP_EDITOR=OFF
```

## 🎮 Getting Started

### First Launch

After building successfully, you should be able to run the game:

```bash
# From build directory
# Windows
.\bin\ChainedDecos.exe

# Linux/macOS
./bin/ChainedDecos
```

### What to Expect

1. **Menu Screen**: The game starts with the main menu visible

   - Use mouse to navigate
   - Select a game mode or load a map

2. **First Time Setup**:

   - The game auto-loads required resources from `resources/` directory
   - Default maps are available in `resources/maps/`
   - If resources are missing, check that `resources/` folder exists in build directory

3. **Controls**:
   - **In Menu**: Mouse navigation
   - **In Game**: `WASD` to move, `Space` to jump, `Mouse` to look around
   - Press `ESC` to return to menu

### Quick Start Guide

**To start playing immediately:**

1. Launch the game (see above)
2. In the menu, select **"Start Game"** or **"Load Map"**
3. Choose a map from the list
4. Wait for the map and models to load
5. Once in-game, use `WASD` and `Space` to move and jump

**If you see errors on first launch:**

- **"Resources not found"**:
  - Resources are auto-copied during build
  - If missing, manually copy `resources/` folder to `build/bin/`
- **"Map not found"**:

  - Check `resources/maps/` contains `.json` map files
  - Try loading from menu map selector

- **"Models not loading"**:
  - Models should auto-load when needed
  - Check `resources/` contains model files (`.glb`, `.gltf`)

### Troubleshooting First Launch

**Game won't start:**

```bash
# Check executable exists
ls -la build/bin/ChainedDecos  # Linux
dir build\bin\ChainedDecos.exe  # Windows

# Run from correct directory
cd build
./bin/ChainedDecos
```

**Black screen or window doesn't appear:**

- Check graphics drivers are up to date
- Verify OpenGL 3.3+ support
- Try windowed mode: `./bin/ChainedDecos -width 1280 -height 720`

**Menu not responding:**

- Make sure you clicked the window to focus it
- Try pressing `F1` to toggle menu visibility

**Performance issues:**

- First launch may be slower due to resource loading
- Subsequent launches will be faster (resources cached)
- Try disabling debug overlays: Press `F2` and `F3` to toggle

> **Tip**: On first launch, the game may take a moment to initialize all systems. Be patient - subsequent launches will be faster.

## 🎯 Usage

### Game Controls

| Key     | Action                              |
| ------- | ----------------------------------- |
| `WASD`  | Move                                |
| `Space` | Jump                                |
| `Shift` | Sprint                              |
| `Mouse` | Look around                         |
| `T`     | Emergency reset (teleport to spawn) |

### Debug & UI

| Key   | Action                               |
| ----- | ------------------------------------ |
| `F1`  | Toggle main menu                     |
| `F2`  | Toggle debug info overlay            |
| `F3`  | Toggle collision debug visualization |
| `ESC` | Pause/return to menu                 |
| `~`   | Toggle developer console             |

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
ChainedDecos.exe -fullscreen -width 1920 -height 1080 -novsync
```

Available options:

- `-width <width>` - Set window width
- `-height <height>` - Set window height
- `-fullscreen` - Start in fullscreen mode
- `-novsync` - Disable VSync
- `-map <mapname>` - Load specific map
- `-dev` - Enable developer mode

### Configuration

Edit `game.cfg` for persistent settings:

- Resolution and display options
- Audio volume and sensitivity
- Control bindings
- Debug settings

## 🧪 Testing

Run the unit tests:

```bash
# From build directory
# Windows
.\bin\tests\ChainedDecosUnitTests.exe

# Linux/macOS
./bin/tests/ChainedDecosUnitTests
```

Key test modules:

- **Physics Tests**: `tests/engine/PhysicsComponentTest.cpp`
- **Collision Tests**: BVH and AABB collision accuracy
- **Integration Tests**: `tests/integration/GameIntegrationTest.cpp`

## 📊 Performance

### System Requirements

- **Minimum**: CPU with SSE2, 4GB RAM, OpenGL 3.3+
- **Recommended**: Modern CPU, 8GB RAM, dedicated GPU

### Optimization Features

- **BVH Collision Detection**: O(log n) collision queries
- **Model Caching**: LRU cache for frequently used assets
- **Parallel Physics**: Multi-threaded physics updates
- **Memory Pooling**: Efficient collision object management

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

- Follow C++23 best practices and RAII principles
- Add unit tests for new features
- Update documentation for API changes
- Use the established code style (similar to Google C++ Style Guide)

## 📝 License

This project is licensed under the MIT License - see the [`LICENSE`](LICENSE) file for details.

## 🙏 Acknowledgments

- **[raylib](https://www.raylib.com/)**: Cross-platform game development library
- **[Dear ImGui](https://github.com/ocornut/imgui)**: Bloat-free graphical user interface
- **[Google Test](https://github.com/google/googletest)**: C++ testing framework
- **Community Contributors**: Thanks to all who have contributed!

---

**Made with ❤️ using raylib, ImGui, and modern C++23**

_Chained Decos - Where momentum meets precision_
