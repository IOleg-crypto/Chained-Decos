#ifndef CH_RENDER_COMMAND_H
#define CH_RENDER_COMMAND_H

#include "raylib.h"

namespace CHEngine
{
class RenderCommand
{
public:
    static void Initialize();
    static void Shutdown();

    static void Clear(Color color);
    static void SetViewport(int x, int y, int width, int height);

    static void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
    static void DrawGrid(int sliceCount, float spacing);

    static void SetBlendMode(int blendMode);
    static void EnableDepthTest();
    static void DisableDepthTest();
    static void EnableBackfaceCulling();
    static void DisableBackfaceCulling();
    static void EnableDepthMask();
    static void DisableDepthMask();
};
} // namespace CHEngine

#endif // CH_RENDER_COMMAND_H
