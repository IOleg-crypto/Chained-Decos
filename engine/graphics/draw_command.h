#ifndef CH_DRAW_COMMAND_H
#define CH_DRAW_COMMAND_H

#include "engine/graphics/environment.h"
#include "engine/scene/components.h"
#include "raylib.h"
#include "render_types.h"
#include "string"
#include "vector"

namespace CHEngine
{
class DrawCommand
{
public:
    static void Init();
    static void Shutdown();

    static RendererState &GetState()
    {
        return s_State;
    }

    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);
    static void ApplyEnvironment(const EnvironmentSettings &settings);

    static void Clear(Color color);
    static void SetViewport(int x, int y, int width, int height);

    static void DrawModel(const std::string &path, const Matrix &transform,
                          const std::vector<MaterialSlot> &overrides);
    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera);
    static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                                float length, Color color);

private:
    static RendererState s_State;
};
} // namespace CHEngine

#endif // CH_DRAW_COMMAND_H
