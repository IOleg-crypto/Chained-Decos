# Repository Guidelines


## Project Structure & Modules
- Core engine: src/Engine
- Game logic: src/Game
- Map editor & tools: src/MapEditor
- Tests: tests (engine/, game/, integration/)
- Assets & configs: resources/, game.cfg, imgui.ini


## Build, Test & Run
- Configure (default Debug): mkdir build; cd build; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
- Configure Release: cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
- Build: cmake --build . --config Debug
- Run game: ./bin/ChainedDecos (Linux/macOS) or .\bin\ChainedDecos.exe (Windows)
- Build & run tests: cmake -DBUILD_TESTS=ON ..; cmake --build .; ctest or ./tests/unit_tests


## Coding Style & Naming
- Language: C++23, header/source split under src
- Formatting: clang-format (.clang-format, 4-space indent, Allman braces, column limit 100)
- Naming: PascalCase for types, camelCase for variables/functions, UPPER_SNAKE_CASE for constants and macros
- Prefer modern C++ (RAII, smart pointers, span, ranges) and avoid raw new/delete in gameplay code


## Testing Guidelines
- Framework: GoogleTest; tests live under tests/ (engine/, game/, integration/ and gtest.cpp entry point)
- Add new tests near the code they cover and keep names descriptive (e.g., Player_Movement_AppliesGravity)
- Run full suite from build/: ctest or ./tests/unit_tests
- Aim to keep regressions covered; add focused tests for bugfixes


