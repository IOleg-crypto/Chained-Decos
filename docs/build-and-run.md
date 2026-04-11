# Build and Run

This page gives the shortest useful path to configure, build, and launch the project.

## Prerequisites

- CMake 3.31 or newer.
- A C++23-capable compiler.
- Ninja for the recommended fast-build path.
- .NET SDK 9.0.x for managed scripting workflows.
- OpenGL 4.3+ drivers.

## Quick Start

1. Initialize submodules.

```bash
git submodule update --init --recursive
```

2. Configure with a preset.

```bash
cmake --preset windows-gcc
cmake --preset windows-clang
cmake --preset linux-clang
```

3. Build.

```bash
cmake --build --preset windows-gcc --parallel
cmake --build --preset windows-clang --parallel
cmake --build --preset linux-clang --parallel
```

4. Run the editor or runtime from the preset bin folder.

```bash
./build/linux-clang/bin/ChainedEditor
.\build\windows-gcc\bin\ChainedEditor.exe
.\build\windows-gcc\bin\ChainedRuntime.exe --project path\to\project.chproject
```

## Notes

- Keep one build directory per generator family.
- If you switch toolchains or presets, reconfigure from a clean build tree.
- Managed artifacts are built through the scripting target when .NET is available.

## Build Outputs

Generated binaries live under build/{preset}/bin.

The editor is the authoring entry point. The runtime is the lighter shipping/testing entry point.