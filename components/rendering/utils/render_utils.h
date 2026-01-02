#ifndef CD_COMPONENTS_RENDERING_UTILS_RENDER_UTILS_H
#define CD_COMPONENTS_RENDERING_UTILS_RENDER_UTILS_H

#include <raylib.h>

// Utility functions for rendering
namespace RenderUtils
{
    // Draw a textured cube using rlgl (based on Raylib example, with corrected UV coordinates)
    // NOTE: Cube position is the center position
    void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color);
}

#endif // CD_COMPONENTS_RENDERING_UTILS_RENDER_UTILS_H





