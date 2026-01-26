#ifndef CH_DRAW_COMMAND_H
#define CH_DRAW_COMMAND_H

#include "engine/scene/components.h"
#include "render_types.h"
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{
class DrawCommand
{
public:
    static void Clear(Color color);
    static void SetViewport(int x, int y, int width, int height);

    static void DrawModel(const std::string &path, const Matrix &transform,
                          const std::vector<MaterialSlot> &overrides);
    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera);
    static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                                float length, Color color);
};
} // namespace CHEngine

#endif // CH_DRAW_COMMAND_H
