#ifndef EDITOR_TYPES_H
#define EDITOR_TYPES_H

#include <cstdint>

// Editor modes
enum class EditorMode : std::uint8_t
{
    SCENE_3D = 0, // 3D scene editing (default)
    UI_DESIGN = 1 // 2D UI design mode
};

// Scene state
enum class SceneState : std::uint8_t
{
    Edit = 0,
    Play = 1,
    Pause = 2
};

// Selection categories
enum class SelectionType : std::uint8_t
{
    NONE = 0,
    WORLD_OBJECT = 1,
    UI_ELEMENT = 2
};

// Gizmo axes for transformation tools
enum class GizmoAxis : std::uint8_t
{
    NONE,
    X,
    Y,
    Z,
    XY,
    XZ,
    YZ
};

// Available editor tools
enum Tool : std::uint8_t
{
    SELECT = 0,                 // Select objects
    MOVE = 1,                   // Move objects
    ROTATE = 2,                 // Rotate objects
    SCALE = 3,                  // Scale objects
    ADD_CUBE = 4,               // Add cube primitive
    ADD_SPHERE = 5,             // Add sphere primitive
    ADD_CYLINDER = 6,           // Add cylinder primitive
    ADD_MODEL = 7,              // Add 3D model
    ADD_SPAWN_ZONE = 8,         // Add player spawn zone
    ADD_SKYBOX = 9,             // Add skybox
    ADD_SKYBOX_CUBEMAP = 10,    // Add skybox cubemap
    ADD_SKYBOX_HDR = 11,        // Add skybox hdr
    ADD_SKYBOX_VS = 12,         // Add skybox vs
    ADD_SKYBOX_FS = 13,         // Add skybox fs
    ADD_SKYBOX_CUBEMAP_VS = 14, // Add skybox cubemap vs
    ADD_SKYBOX_CUBEMAP_FS = 15  // Add skybox cubemap fs
};

#endif // EDITOR_TYPES_H
