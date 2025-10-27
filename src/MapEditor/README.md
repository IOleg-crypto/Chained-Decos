# ChainedDecos MapEditor

A professional 3D level editor built on the ChainedDecos game engine, featuring advanced terrain editing, lighting, materials, and object management tools.

## Features

### 🏔️ Terrain Editing
- **Heightmap-based terrain sculpting** with multiple brush types
- **Real-time terrain mesh generation** with normal calculation
- **Texture painting system** with multiple layers
- **Brush controls**: Size, strength, falloff for precise editing
- **Heightmap import/export** support

### 💡 Advanced Lighting System
- **Multiple light types**: Directional, Point, Spot, Area lights
- **Visual light gizmos** for easy manipulation
- **Shadow mapping** with configurable quality settings
- **Environment lighting** with cubemap support
- **Ambient lighting controls**

### 🎨 Material Editor
- **PBR material system** with Standard, Metallic, Glass, Emissive, Transparent materials
- **Texture management** for all PBR maps (Diffuse, Normal, Metallic, Roughness, etc.)
- **Material properties**: Base color, metallic, roughness, transparency, refractive index
- **Real-time material preview**
- **Material library** with save/load functionality

### 📦 Prefab System
- **Reusable object templates** with metadata
- **Prefab categories and tagging** for organization
- **Thumbnail generation** and preview system
- **Instance management** with transform controls
- **Prefab library** with search and filtering

### ✨ Particle Effects Editor
- **Multiple emitter types**: Point, Box, Sphere, Circle, Cone, Mesh
- **Advanced particle properties**: Physics, animation, rendering
- **Real-time particle simulation**
- **Resource management** for textures and models

### 📏 Measurement Tools & Snapping
- **Measurement types**: Distance, Angle, Area, Volume, Coordinates
- **Advanced snapping modes**: Grid, Vertex, Edge, Face, Object Center, World Axes
- **Visual measurement tools**: Ruler, Protractor, Area calculator
- **Configurable grid system**

## Architecture

### Project Structure
```
MapEditor/
├── main.cpp                    # Application entry point
├── Application.h/cpp          # Main application class
├── Editor/                    # Core editor functionality
│   ├── Editor.h/cpp          # Main editor class
│   ├── EnhancedEditor.h      # Advanced editor features
│   ├── TerrainEditor.h/cpp   # Terrain editing system
│   ├── LightingSystem.h/cpp  # Lighting management
│   ├── MaterialEditor.h/cpp  # Material management
│   ├── PrefabSystem.h/cpp    # Prefab management
│   ├── ParticleEditor.h/cpp  # Particle effects
│   ├── MeasurementTools.h/cpp # Measurement & snapping
│   └── ...                   # Other editor components
├── MapFileManager/           # File I/O management
└── CMakeLists.txt           # Build configuration
```

### Dependencies
- **ChainedDecosEngine**: Core engine functionality
- **raylib**: Graphics and windowing
- **ImGui**: User interface

## Building

### Prerequisites
- CMake 3.16 or higher
- C++17 compatible compiler
- ChainedDecosEngine built and installed

### Build Instructions
```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build MapEditor
cmake --build build --target ChainedDecosMapEditor

# Run MapEditor
cmake --build build --target run_mapeditor
```

### Build Options
- `BUILD_MAPEDITOR_TESTS`: Enable unit tests (default: OFF)
- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo, MinSizeRel

## Usage

### Basic Controls
- **WASD**: Navigate in 3D view
- **Mouse**: Rotate camera (right-click) and select objects (left-click)
- **Ctrl+S**: Save map
- **Ctrl+O**: Load map
- **Ctrl+Z/Ctrl+Y**: Undo/Redo

### Terrain Editing
1. Select terrain brush tool from toolbar
2. Adjust brush settings (size, strength, type)
3. Click and drag on terrain to sculpt
4. Use different brush types for various effects

### Lighting Setup
1. Add lights from the lighting panel
2. Position and configure light properties
3. Enable shadows for realistic lighting
4. Set up environment lighting for ambient effects

### Material Assignment
1. Select object in scene
2. Choose material from material library
3. Adjust material properties as needed
4. Apply textures for realistic surfaces

## File Formats

### Supported Formats
- **Maps**: `.map` (JSON-based)
- **Prefabs**: `.prefab` (custom binary format)
- **Materials**: `.mat` (JSON-based)
- **Heightmaps**: `.raw` (16-bit grayscale)
- **Textures**: PNG, JPG, BMP, TGA, DDS

### File Organization
```
Project/
├── Maps/              # Map files
├── Prefabs/          # Reusable object templates
├── Materials/        # Material libraries
├── Textures/         # Texture assets
└── Heightmaps/       # Terrain height data
```

## Advanced Features

### Layer Management
- Organize objects into layers
- Control visibility and locking per layer
- Layer-based filtering and selection

### Undo/Redo System
- Full operation history
- Branching undo for complex operations
- Configurable history size

### Multiple Selection
- Select multiple objects simultaneously
- Batch operations on selected objects
- Selection filtering and grouping

## Performance

### Optimization Features
- **Level-of-detail (LOD)** for distant objects
- **Frustum culling** for off-screen objects
- **Occlusion culling** for hidden geometry
- **Texture streaming** for large textures

### Performance Settings
- Adjustable shadow quality
- Particle count limits
- Grid density controls
- Render distance settings

## Contributing

### Code Organization
- **Modular design** with clear separation of concerns
- **Consistent naming conventions**
- **Comprehensive documentation**
- **Unit tests** for critical functionality

### Adding New Features
1. Create new editor component in `Editor/` directory
2. Implement interface following existing patterns
3. Add to CMakeLists.txt build configuration
4. Update documentation

## Troubleshooting

### Common Issues
- **Build errors**: Ensure ChainedDecosEngine is built first
- **Missing textures**: Check resource paths in configuration
- **Performance issues**: Adjust quality settings in options

### Debug Information
- Enable debug logging in configuration
- Check console output for error messages
- Use built-in diagnostic tools

## License

This project is part of the ChainedDecos game engine and follows the same licensing terms.

## Support

For issues and questions:
- Check existing documentation
- Review build configuration
- Examine console output for error messages
- Consult the main engine documentation for dependency issues