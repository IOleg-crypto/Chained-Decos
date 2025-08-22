# Repository Info

- name: Chained Decos
- root: d:\gitnext\Chained Decos
- language: C++ (Raylib, ImGui, GoogleTest)
- build: CMake
- targets:
  - app: main game executable (Raylib)
  - tests: GoogleTest unit tests in /tests
- notable libs: raylib, rlImGui, imgui, nlohmann_json, googletest
- run:
  - Windows: build via CMake presets or cmake-build-* folders; executable in build/bin
  - Tests: `tests/run_tests.bat` or CTest in build directory

## Key paths

- src/Engine: main engine loop, input, rendering
- src/Collision: hybrid AABB/Octree collision system
- src/Model: model loading, caching, LOD
- src/Player: player logic, physics helpers
- src/Render: render manager and debug overlays
- resources/: models, textures, icons
- tests/: unit tests and gtest entrypoint

## Notes

- Engine::Run ends when WindowShouldClose() or exit requested; destructor logs shutdown steps
- Collision::DetermineOptimalCollisionType chooses AABB for 0..100 tris, OCTREE otherwise
- EnsureOctree builds lazily only when needed
- F2/F3 toggle debug overlays, F4 requests exit, F11 toggles fullscreen
