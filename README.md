# Chained Decos

[![C++](https://img.shields.io/badge/language-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)](https://isocpp.org/) [![English](https://img.shields.io/badge/lang-English-blue?logo=github)](README.md)

Chained Decos is a fast-paced parkour game with both singleplayer and multiplayer modes, built in modern C++ (C++20) using the [raylib](https://www.raylib.com/) library. The project also leverages [ImGui](https://github.com/ocornut/imgui) and [rlImGui](https://github.com/raylib-extras/rlImGui) for its in-game UI and debugging tools.

> [!IMPORTANT]  
> The game is currently in active development, with its core engine undergoing significant restructuring and finalization. Expect frequent updates and changes.

> [!WARNING]  
> Use previous commits to load project
---
> [!NOTE]
> **ChainedEditor** is a level/map editor for the game. It allows you to create and edit 3D scenes, place game objects, and save maps for use within the game engine. This editor is also under active development and its features may evolve rapidly.
> ![2025-08-09-150557.png](https://i.postimg.cc/x8P86rsH/2025-08-09-150557.png)

## 🚀 Features

-   **Dynamic Parkour Gameplay:** Run, jump, vault, and chain together moves to overcome challenging levels.
-   **Multiplayer & Singleplayer:** (Currently Planning) Experience the game solo or compete with friends online.
-   **Modern C++20 Codebase:** Designed with a clean, modular, and extensible architecture.
-   **ImGui-powered UI:** Intuitive in-game menus and robust debugging tools utilizing ImGui + rlImGui.
-   **Smooth Controls:** Responsive movement and physics for an immersive gameplay experience.
-   **Custom Levels:** (Currently Planning) Create and share your own parkour challenges with the community.

> [!IMPORTANT]
>
> ## Early Preview
>
> ![2025-08-09-145416.png](https://i.postimg.cc/43rgt81g/2025-08-09-145416.png)
> ![2025-08-09-145341.png](https://i.postimg.cc/j5zGfR5H/2025-08-09-145341.png)

## 🛠️ Getting Started

### Prerequisites

To build and run Chained Decos, you will need:

-   **C++20 Compiler:** A compiler that fully supports the C++20 standard (e.g., GCC 10+, Clang 11+, MSVC 19.28+).
-   **CMake:** Version 3.20 or newer (for project configuration and build system generation).
-   **Git:** For cloning the repository and its submodules.

### Dependency Management (FetchContent)

This project uses CMake's `FetchContent` module to automatically download and configure all external dependencies, including `raylib`, `ImGui`, `rlImGui`, `nlohmann/json`, and `GoogleTest` (for testing).

This means you **do not** need to install these libraries manually on your system. CMake will handle everything during the configuration phase, downloading the necessary source code directly into your build environment. Just ensure you have Git and CMake set up.

### Building the Project

Follow these steps to set up and build the project:

1.  **Clone the Repository (with submodules):**
    Ensure you clone the repository recursively to fetch all dependencies.

   ```bash
   git clone --recurse-submodules https://github.com/IOleg-crypto/Chained-Decos.git
   cd ChainedDecos
   ```

    If you cloned without `--recurse-submodules`, you can initialize them later:
    ```bash
    git submodule update --init --recursive
    ```

2.  **Create a Build Directory and Configure CMake:**

    ```bash
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug # or Release, MinSizeRel, RelWithDebInfo
    ```
    *   **Note for Windows (MinGW/MSYS2 users):** Ensure your `PATH` variable includes your MinGW/MSYS2 `bin` directory (e.g., `C:\msys64\mingw64\bin`). CMake will detect your compilers automatically.
    *   **Note for Visual Studio users:** CMake will typically generate Visual Studio solution files. You can then open the generated `.sln` file in Visual Studio.

3.  **Build the Project:**
    This command will compile all source files and link the executable.

    ```bash
    cmake --build . --config Debug # Specify configuration (Debug/Release)
    ```

4.  **Run the Game:**
    After a successful build, the executable will be located in the build directory, typically `build/src/Game/`.

    ```bash
    ./src/Game/ChainedDecosGame # On Linux/macOS
    .\src\Game\ChainedDecosGame.exe # On Windows (adjust path if different)
    ```

### Running Tests

To build and run the unit tests (powered by GoogleTest), configure CMake with the `BUILD_TESTS` option enabled:

   ```bash
# From your build directory (e.g., ChainedDecos/build)
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
   cmake --build .
ctest # To run the tests
```

### ChainedMapEditor

The `ChainedMapEditor` is a separate executable that allows you to create and edit 3D scenes for the game. This editor is built as an optional component.

To build and run the `ChainedMapEditor`, you need to enable the `BUILD_MAP_EDITOR` CMake option:

1.  **Configure CMake with `BUILD_MAP_EDITOR` enabled:**

    ```bash
    # From your build directory (e.g., ChainedDecos/build)
    cmake .. -DBUILD_MAP_EDITOR=ON -DCMAKE_BUILD_TYPE=Debug
    ```

2.  **Build the Project:**

    ```bash
    cmake --build . --config Debug # Specify configuration (Debug/Release)
    ```

3.  **Run the Editor:**
    The editor executable will be located in the build directory, typically `build/src/MapEditor/`.

    ```bash
    ./src/MapEditor/ChainedDecosMapEditor # On Linux/macOS
    .\src\MapEditor\ChainedDecosMapEditor.exe # On Windows (adjust path if different)
    ```

### Cleaning the Build Cache (Troubleshooting)

If you encounter build errors (e.g., "No rule to make target" or issues after modifying CMakeLists.txt files), it's often helpful to clean your CMake cache and rebuild.

   ```bash
# From the root of your project (ChainedDecos/)
rm -rf build  # Deletes the entire build directory
# For Windows: rmdir /s /q build

# Then, re-run the build steps from "Create a Build Directory and Configure CMake"
   ```

---

## 💻 Recommended IDEs

You can use any C++ IDE that supports CMake. Here are some popular options:

-   [**CLion**](https://www.jetbrains.com/clion/) – Full-featured C++ IDE with excellent CMake integration.
-   [**Visual Studio**](https://visualstudio.microsoft.com/) (Windows) – Powerful IDE with built-in CMake support.
-   [**Visual Studio Code**](https://code.visualstudio.com/) – Lightweight editor; use with the [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) and [C/C++ extensions](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).
-   [**Qt Creator**](https://www.qt.io/product/development-tools) – Good CMake support and cross-platform.
-   [**KDevelop**](https://www.kdevelop.org/) – Open-source IDE with CMake integration (Linux).

You can also use any text editor and build from the command line with CMake.

---

## 🎮 Controls

-   **WASD:** Move Character
-   **Space:** Jump
-   **Shift:** Sprint
-   **Mouse:** Look Around
-   **F1:** Toggle Main Menu
-   **F2:** Toggle Debug Info
-   **F3:** Toggle Collision Debug Visualization
-   **F4:** Request Game Exit
-   **F11:** Toggle Fullscreen

---

## 🤝 Contributing

Pull requests are welcome! For major changes, please open an issue first to discuss what you would like to change.

---

## ❓ FAQ

**Q: How do I install raylib, ImGui, and rlImGui?**

-   The project uses CMake's `FetchContent` module to automatically download and configure `raylib`, `ImGui`, `rlImGui`, and `nlohmann/json`. You do not need to install them manually. Just ensure you have Git and CMake set up.

**Q: Does the game support custom levels?**

-   Custom level support is planned for future releases. The `ChainedEditor` is currently in development to facilitate this.

**Q: Can I use this project as a base for my own game?**

-   Yes! The code is open source under the MIT license, making it suitable as a base for your own projects.

---

## 📚 Resources

-   [raylib documentation](https://www.raylib.com/cheatsheet/cheatsheet.html)
-   [ImGui documentation](https://github.com/ocornut/imgui/wiki)
-   [rlImGui GitHub](https://github.com/raylib-extras/rlImGui)
-   [nlohmann/json GitHub](https://github.com/nlohmann/json)

---

## 📝 License

[MIT](LICENSE)

---
*Made with ❤️ using raylib, ImGui, and rlImGui*
