# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

Chained Decos is a fast-paced parkour game built with modern C++20 and [raylib](https://www.raylib.com/). It features a custom physics engine with BVH collision detection, modular architecture, and comprehensive debugging tools.

> [!IMPORTANT]
> This project is under active development. Expect frequent changes and engine refactors.

## Features

### Core Gameplay
- **3D Parkour Mechanics**: Fluid movement system with momentum-based physics
- **Dynamic Map Generation**: Procedurally generated parkour courses with multiple difficulty levels
- **Real-time Physics**: Advanced collision detection and response system
- **Multiple Game Modes**: Test, Easy, Medium, Hard, and Speedrun difficulties
- **Timer System**: Cross-platform timer display with dynamic font scaling

### Development Tools
- **Integrated Map Editor**: Full-featured level editor with real-time preview
- **Particle System(Planning)**: Dynamic visual effects for enhanced gameplay
- **Lighting System(Planning)**: Advanced lighting with multiple light sources and shadows
- **Material Editor(Planning)**: Comprehensive material system for visual customization

### Technical Features
- **High-Performance Engine**: Optimized rendering and physics systems
- **Cross-Platform Support**: Windows, macOS, and Linux compatibility
- **Modular Architecture**: Clean separation of engine, game logic, and tools
- **Comprehensive Testing**: Unit tests
- **Advanced Debugging**: Collision visualization, performance metrics, real-time tweaking
- **Developer Console**: In-game command console with Source engine-style commands

## Installation

### Prerequisites
- **CMake**: 3.20+
- **C++20 compiler**: GCC 10+, Clang 11+, or compatible
- **Git**

### Steps
1. **Clone the repository**:
   ```bash
   git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
   cd Chained-Decos
   ```

2. **Configure and build**:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug  # Or Release
   cmake --build .
   ```

3. **Run the game**:
   - Windows: `.\src\Game\ChainedDecosGame.exe`
   - Linux/macOS: `./src/Game/ChainedDecosGame`

For the map editor, add `-DBUILD_MAP_EDITOR=ON` to the cmake command.

## Usage

### Controls
- **WASD**: Move
- **Space**: Jump
- **Shift**: Sprint
- **Mouse**: Look around
- **T**: Emergency reset (teleport to spawn)

### Debug & UI
- **F1**: Toggle main menu
- **F2**: Toggle debug info overlay
- **F3**: Toggle collision debug visualization
- **ESC**: Pause/return to menu
- **~ (Tilde)**: Toggle developer console

### Console Commands
- `help` - Show available commands
- `clear` - Clear console output
- `quit/exit` - Exit game
- `fps` - Show current FPS
- `res <width>x<height>` - Set resolution
- `fullscreen` - Toggle fullscreen mode
- `vsync <on/off>` - Toggle VSync

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

- **Tips**: Build momentum for longer jumps, plan ahead, and use all movement options.

### Configuration
Edit `game.cfg` for settings like resolution, volume, and sensitivity.

## Contributing
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

*Made with ❤️ using raylib, ImGui, and modern C++20*
