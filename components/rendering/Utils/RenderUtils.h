#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#include <raylib.h>

// Utility functions for rendering
namespace RenderUtils
{
    // Draw a textured cube using rlgl (based on Raylib example, with corrected UV coordinates)
    // NOTE: Cube position is the center position
    void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color);
}

#endif // RENDERUTILS_H





