#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include <raylib.h>

namespace CH
{
class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void DrawGrid(int slices, float spacing);
    static void DrawLine(Vector3 start, Vector3 end, Color color);

    // For 2D / UI
    static void BeginUI();
    static void EndUI();

private:
    Renderer() = default;
};
} // namespace CH

#endif // CH_RENDERER_H
