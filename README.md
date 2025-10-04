# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

Chained Decos is a fast-paced parkour game built with modern C++20 and [raylib](https://www.raylib.com/). It features a custom physics engine with BVH collision detection, modular architecture, and comprehensive debugging tools.

> [!IMPORTANT]
> This project is under active development. Expect frequent changes and engine refactors.

> [!WARNING]
> Configuration can occasionally be unstable. If you hit issues, see the Troubleshooting section.

## 🔧 Recent Updates

### Bug Fixes
- **Fixed collision bug**: Players can no longer fall through the ground after touching collision objects
- **Improved ground detection**: Enhanced physics system to better detect ground after collisions
- **Fixed settings menu overlap**: Video settings now display current and selected values without overlapping
- **Added collision response validation**: Prevents invalid player movements that could cause teleportation

### New Features
- **Developer Console**: In-game command console with Source engine-style commands
- **Command Line Support**: Launch options like `-fullscreen`, `-width`, `-height`, etc.
- **Game Pause on Console**: Game automatically pauses when console is open
- **Enhanced Settings Display**: Settings menu shows both current system values and selected options

---

## 🚀 Features

### Core Gameplay
- **3D Parkour Mechanics**: Fluid movement system with momentum-based physics
- **Dynamic Map Generation**: Procedurally generated parkour courses with multiple difficulty levels
- **Real-time Physics**: Advanced collision detection and response system
- **Multiple Game Modes**: Test, Easy, Medium, Hard, and Speedrun difficulties
- **Timer System**: Cross-platform timer display with dynamic font scaling

### Development Tools
- **Integrated Map Editor**: Full-featured level editor with real-time preview
- **Particle System**: Dynamic visual effects for enhanced gameplay
- **Lighting System**: Advanced lighting with multiple light sources and shadows
- **Material Editor**: Comprehensive material system for visual customization

### Technical Features
- **High-Performance Engine**: Optimized rendering and physics systems
- **Cross-Platform Support**: Windows, macOS, and Linux compatibility
- **Modular Architecture**: Clean separation of engine, game logic, and tools
- **Comprehensive Testing**: Unit tests and performance benchmarks
- **Advanced Debugging**: Collision visualization, performance metrics, real-time tweaking
- **Developer Console**: In-game command console with Source engine-style commands

---

## 🛠️ Quick Start

### 1) Install prerequisites (Compiler, CMake, Git)

- **Minimum versions**
  - **CMake**: 3.20+
  - **C++20 compiler**: GCC 10+, Clang 11+, or MSVC 19.28+ (Visual Studio 2019 16.8+)

#### Windows (choose one toolchain)
- Option A — MSVC (recommended for Visual Studio/CLion)
  1. Install Visual Studio (or "Build Tools for Visual Studio") with the "Desktop development with C++" workload
  2. Use the x64 toolchain ("x64 Native Tools Command Prompt for VS 2022") or let CMake/IDE detect it automatically

- Option B — MSYS2 MinGW-w64 (for CMake + Ninja/Make workflows)
  1. Install MSYS2: https://www.msys2.org/
  2. Open the "MSYS2 MinGW x64" shell and run:
     ```bash
     pacman -Syu
     pacman -S --needed mingw-w64-x86_64-toolchain git cmake ninja
     ```
  3. Add to PATH in Windows: `C:\msys64\mingw64\bin`

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake git
# Optional newer clang
sudo apt install -y clang
```

#### Fedora
```bash
sudo dnf install -y gcc-c++ cmake git
# Or clang
sudo dnf install -y clang
```

#### Arch/Manjaro
```bash
sudo pacman -S --needed base-devel cmake git
# Optional clang
sudo pacman -S clang
```

#### macOS
- Install Xcode Command Line Tools:
```bash
xcode-select --install
```
- Homebrew (recommended):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake git
# Optional newer compiler
brew install llvm
```

### 2) Clone the repository
```bash
git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
cd Chained-Decos
# If you cloned without submodules
# git submodule update --init --recursive
```

### 3) Configure and build with CMake
This project uses CMake `FetchContent` to automatically fetch dependencies (raylib, ImGui, rlImGui, nlohmann_json, GoogleTest). No manual installs required beyond CMake and Git.

- Generic single-config generators (Make/Ninja):
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug   # Or Release/RelWithDebInfo/MinSizeRel
cmake --build . --config Debug
```

- Windows (Visual Studio multi-config):
```powershell
# inside build folder
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
```

- Windows (MSYS2 MinGW + Ninja):
```bash
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### 4) Run
Executables usually end up under `build/src/...` (exact path depends on generator/config).

- Windows:
```powershell
# Examples (adjust to your generator/output tree)
.\src\Game\ChainedDecosGame.exe
.\src\MapEditor\ChainedDecosMapEditor.exe  # if built
```

- Linux/macOS:
```bash
./src/Game/ChainedDecosGame
# Or the editor (if built)
./src/MapEditor/ChainedDecosMapEditor
```

> Tip: The repo may include pre-created `cmake-build-*` folders or a `clean_and_configure.bat` helper. You can use them for convenience.

---

## 🧪 Tests
```bash
# from your build directory
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest
```

---

## 🧰 Map Editor (ChainedMapEditor)
Enable the editor via `BUILD_MAP_EDITOR=ON`:
```bash
cmake .. -DBUILD_MAP_EDITOR=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```
Run (path depends on your generator):
```bash
./src/MapEditor/ChainedDecosMapEditor      # Linux/macOS
.\src\MapEditor\ChainedDecosMapEditor.exe # Windows
```

---

## 🧹 Troubleshooting
- **CMake cannot find a compiler/toolchain**
  - Windows (MSVC): open "x64 Native Tools Command Prompt for VS 2022" and reconfigure
  - Windows (MSYS2): ensure `C:\msys64\mingw64\bin` is in PATH; use `-G Ninja`
  - Linux/macOS: verify `g++ --version` or `clang++ --version`
- **Broken CMake cache / changed CMakeLists**
  - Delete `build` and reconfigure from scratch
- **Linker errors for raylib/ImGui**
  - Ensure internet connectivity on first configure (FetchContent downloads deps)
  - Clear `build` and retry
- **Wrong architecture**
  - Use consistent x64 across generator and toolchain

Clean build directory commands:
- Linux/macOS:
```bash
rm -rf build
```
- Windows PowerShell:
```powershell
Remove-Item -Recurse -Force build
```

---

## 💻 Recommended IDEs
- **Visual Studio** (Windows) — excellent CMake integration
- **CLion** — great CMake workflow
- **VS Code** — install the CMake Tools and C/C++ extensions

---

## 🎮 Controls

### Gameplay
- **WASD**: Move
- **Space**: Jump
- **Shift**: Sprint
- **Mouse**: Look around
- **T**: Emergency reset (teleport to spawn)

### Debug & UI
- **F1**: Toggle main menu
- **F2**: Toggle debug info overlay
- **F3**: Toggle collision debug visualization
- **F4**: Exit game
- **F11**: Toggle fullscreen
- **ESC**: Pause/return to menu
- **~ (Tilde)**: Toggle developer console

### Console Commands
Open the console with **~** and type commands:
- `help` - Show available commands
- `clear` - Clear console output
- `quit/exit` - Exit game
- `fps` - Show current FPS
- `map <name>` - Load specific map
- `res <width>x<height>` - Set resolution
- `fullscreen` - Toggle fullscreen mode
- `vsync <on/off>` - Toggle VSync
- `noclip` - Toggle noclip mode (planned)

### Command Line Arguments
Launch with command line options:
```bash
ChainedDecos.exe -fullscreen -width 1920 -height 1080 -novsync
```

Available options:
- `-width <width>` - Set window width
- `-height <height>` - Set window height
- `-fullscreen` - Start in fullscreen mode
- `-windowed` - Start in windowed mode
- `-noborder` - Start in borderless window mode
- `-novsync` - Disable VSync
- `-fps <fps>` - Set target FPS (0 for unlimited)
- `-map <mapname>` - Load specific map
- `-dev` - Enable developer mode
- `-help` - Show all available options

## 🎯 Gameplay Guide

### Game Modes

1. **Test Mode**: Basic parkour course for testing and development
2. **Easy**: Beginner-friendly course with forgiving platform placement
3. **Medium**: Balanced difficulty with challenging but fair obstacles
4. **Hard**: Expert-level course requiring precise movement and timing
5. **Speedrun**: Time-based challenge with optimal route design

### Tips for Success

- **Momentum is Key**: Build and maintain speed for longer jumps
- **Look Ahead**: Plan your next few moves while executing current ones
- **Use All Movement Options**: Sprint, jump, and chain movements together
- **Learn Platform Layouts**: Each difficulty has unique platform arrangements

## ⚙️ Configuration

### Game Settings (`game.cfg`)

```ini
# Graphics settings
window_width=1920
window_height=1080
fullscreen=false
vsync=true

# Audio settings
master_volume=0.8
music_volume=0.6
sfx_volume=0.9

# Gameplay settings
mouse_sensitivity=1.0
invert_mouse=false
show_fps=true
show_timer=true
```

### Build Configuration Options

The CMake build system supports extensive configuration:

```bash
# Development build with debug info
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_INFO=ON

# Optimized release build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_OPTIMIZATIONS=ON

# Build with testing and benchmarks
cmake .. -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=ON

# Enable profiling and sanitizers
cmake .. -DENABLE_PROFILING=ON -DENABLE_SANITIZERS=ON

# Unity build for faster compilation
cmake .. -DENABLE_UNITY_BUILD=ON
```

---

## 🏗️ Architecture

### Engine Layer
- **PhysicsComponent**: Gravity, velocity, jump mechanics
- **CollisionSystem**: BVH tree construction and traversal
- **CollisionManager**: Collision detection and response
- **ModelLoader**: 3D model loading with collision generation
- **RenderManager**: Rendering pipeline with debug visualization

### Game Layer
- **Player**: Movement, input handling, collision integration
- **Game**: Main game loop, state management
- **MapEditor**: Level creation and object placement tools

### Key Design Patterns
- **Factory Pattern**: `GroundColliderFactory` for consistent ground creation
- **Component System**: Modular player components (Movement, Input, Collision, Model)

---

## ❓ FAQ
- **Do I need to install raylib/ImGui/rlImGui manually?**
  - No. CMake FetchContent downloads and builds dependencies automatically.
- **How does the collision system work?**
  - BVH trees for precise mesh collisions, AABB for performance. Ground uses AABB-only for stability.
- **Can I add custom models?**
  - Yes, use the map editor or add models to `resources/` and configure in `models.json`.
- **Performance tips?**
  - Enable collision debug (F3) to visualize collision complexity. Use AABB for simple objects, BVH for complex meshes.
- **License?** — MIT (see LICENSE)

---

## 📊 Performance

### Optimization Features

- **Unity Build**: Reduced compilation times for faster development
- **Profiling Support**: Built-in performance analysis tools
- **Memory Management**: Smart pointers and efficient allocation strategies
- **Multithreading**: Parallel processing where beneficial
- **BVH Optimization**: Efficient collision detection with bounding volume hierarchies

### Performance Tips

- **Collision Visualization**: Use F3 to visualize collision complexity
- **AABB for Simple Objects**: Use axis-aligned bounding boxes for simple shapes
- **BVH for Complex Meshes**: Use bounding volume hierarchies for detailed models
- **LOD System**: Level-of-detail rendering for distant objects (planned)

### Benchmarks

Run performance benchmarks:
```bash
cmake .. -DBUILD_BENCHMARKS=ON
cmake --build .
./bin/benchmarks
```

## 📦 Packaging and Distribution

### Creating Release Packages

```bash
# Configure for packaging
cmake .. -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR=ZIP

# Build and package
cmake --build . --target package
```

### Supported Platforms

- **Windows**: ZIP and NSIS installer packages
- **macOS**: Drag-and-drop application bundle
- **Linux**: TAR.GZ and DEB packages

### Installation

After packaging, users can:

1. **Windows**: Run the NSIS installer or extract the ZIP archive
2. **macOS**: Drag the .app bundle to Applications folder
3. **Linux**: Install the DEB package or extract TAR.GZ to preferred location

## 🔧 API Documentation

### Engine Architecture

#### Core Systems
- **PhysicsComponent**: Handles gravity, velocity, and jump mechanics
- **CollisionSystem**: Manages BVH tree construction and traversal
- **CollisionManager**: Coordinates collision detection and response
- **ModelLoader**: Loads 3D models with automatic collision generation
- **RenderManager**: Manages rendering pipeline with debug visualization

#### Key Classes

```cpp
// Example: Player movement integration
class Player {
    std::shared_ptr<PhysicsComponent> physics;
    std::shared_ptr<CollisionComponent> collision;
    std::shared_ptr<ModelComponent> model;
};

// Example: Map generation
class ParkourMapGenerator {
    void GeneratePlatforms(DifficultyLevel level);
    void SetupCollisions();
    void AddVisualEffects();
};
```

### Extension Points

#### Adding New Game Modes
1. Create new difficulty class inheriting from `BaseDifficulty`
2. Implement platform generation logic
3. Add collision setup for new objects
4. Register with the game mode system

#### Custom Physics Components
1. Inherit from `PhysicsComponent`
2. Override movement calculation methods
3. Add custom collision response logic
4. Register with the component system

## 📚 Resources

### Official Documentation
- [raylib Game Development Framework](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [ImGui Immediate Mode GUI](https://github.com/ocornut/imgui/wiki)
- [rlImGui Raylib ImGui Integration](https://github.com/raylib-extras/rlImGui)
- [nlohmann/json Modern C++ JSON Library](https://github.com/nlohmann/json)

### Technical References
- [Bounding Volume Hierarchy (BVH)](https://en.wikipedia.org/wiki/Bounding_volume_hierarchy)
- [Axis-Aligned Bounding Box (AABB)](https://en.wikipedia.org/wiki/Minimum_bounding_box#Axis-aligned_minimum_bounding_box)
- [Modern C++ Best Practices](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [CMake Documentation](https://cmake.org/documentation/)

---

## 🤝 Contributing
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

*Made with ❤️ using raylib, ImGui, and modern C++20*