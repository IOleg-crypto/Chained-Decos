#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/math_types.h"
#include "engine/renderer/material.h"
#include "engine/scene/scene.h"
#include <string>

#include "render_types.h"

namespace CHEngine
{
class Render
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const std::vector<struct MaterialSlot> &overrides);
    static void DrawModel(Ref<class ModelAsset> asset, const Matrix &transform,
                          const std::vector<struct MaterialSlot> &overrides);
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const MaterialInstance &material);
    static void DrawModel(const std::string &path, const Matrix &transform, Color tint = WHITE);

    static void DrawScene(Scene *scene,
                          const DebugRenderFlags *debugFlags = nullptr); // To be removed soon

    // Lighting & Environment
    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);
    static void ApplyEnvironment(const struct EnvironmentSettings &settings);

    static void DrawSkybox(const struct SkyboxComponent &skybox, const Camera3D &camera);
    static void DrawSkybox(const struct EnvironmentSettings &settings, const Camera3D &camera);

    // For 2D / UI
    static void BeginUI();
    static void EndUI();

private:
    static RendererState s_State;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
