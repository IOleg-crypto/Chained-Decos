# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md) 

Chained Decos is a fast-paced parkour game with both singleplayer and multiplayer modes, built in modern C++ (C++20) using the [raylib](https://www.raylib.com/) library. The project also leverages [ImGui](https://github.com/ocornut/imgui) and [rlImGui](https://github.com/raylib-extras/rlImGui) for in-game UI.

> [!IMPORTANT]  
> The game is not in a finished state, as its game engine is being finalized.

---
> [!NOTE]
> **ChainedEditor** is a level/map editor for a game currently in development.  
> It allows you to create and edit 3D scenes, place game objects, and save maps  
> for later use within the game engine.
> Note: This editor is still under active development and may change frequently.
> [![2025-08-09-150557.png](https://i.postimg.cc/x8P86rsH/2025-08-09-150557.png)]

## 🚀 Features
- **Dynamic Parkour Gameplay:** Run, jump, vault, and chain together moves to overcome challenging levels.
- **Multiplayer & Singleplayer:** Play solo or compete with friends online.
- **Modern C++20 Codebase:** Clean, modular, and extensible architecture.
- **ImGui-powered UI:** In-game menus and debugging tools using ImGui + rlImGui.
- **Smooth Controls:** Responsive movement and physics for an immersive experience.
- **Custom Levels:** *(Planned)* Create and share your own parkour challenges.

> [!IMPORTANT] 
> ## Early Preview
> [![2025-08-09-145416.png](https://i.postimg.cc/43rgt81g/2025-08-09-145416.png)]
> [![2025-08-09-145341.png](https://i.postimg.cc/j5zGfR5H/2025-08-09-145341.png)]

## 🛠️ Getting Started


### Prerequisites
- C++20 or newer
- [raylib](https://www.raylib.com/)
- [ImGui](https://github.com/ocornut/imgui) + [rlImGui](https://github.com/raylib-extras/rlImGui) (adapter for raylib)
- CMake (recommended)

### Building
1. Clone the repository:
   ```bash
   git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
   cd ChainedDecos
   ```
2. Build with CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```
3. Run the game:
   ```bash
   ./ChainedDecos
   ```

---

## 💻 Recommended IDEs

You can use any C++ IDE that supports CMake. Here are some popular options:

- [**CLion**](https://www.jetbrains.com/clion/) – Full-featured C++ IDE with excellent CMake integration.
- [**Visual Studio**](https://visualstudio.microsoft.com/) (Windows) – Powerful IDE with built-in CMake support.
- [**Visual Studio Code**](https://code.visualstudio.com/) – Lightweight editor; use with the CMake Tools and C++ extensions.
- [**Qt Creator**](https://www.qt.io/product/development-tools) – Good CMake support and cross-platform.
- [**KDevelop**](https://www.kdevelop.org/) – Open-source IDE with CMake integration (Linux).

You can also use any text editor and build from the command line with CMake.

---

## 🎮 Controls
- **WASD:** Move
- **Space:** Jump
- **Shift:** Sprint
- **Mouse:** Look around

---

## 🤝 Contributing
Pull requests are welcome! For major changes, please open an issue first to discuss what you would like to change.

---

## ❓ FAQ
**Q: How do I install raylib, ImGui, and rlImGui?**
- See the [raylib installation guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) and the [rlImGui README](https://github.com/raylib-extras/rlImGui#installation).

**Q: Does the game support custom levels?**
- Custom level support is planned for future releases.

**Q: Can I use this project as a base for my own game?**
- Yes! The code is open source under the MIT license.

---

## 📚 Resources
- [raylib documentation](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [ImGui documentation](https://github.com/ocornut/imgui/wiki)
- [rlImGui GitHub](https://github.com/raylib-extras/rlImGui)

---

## 📝 License
[MIT](LICENSE)

---
*Made with ❤️ using raylib, ImGui, and rlImGui*
