#ifndef CH_VISUALS_H
#define CH_VISUALS_H

#include "engine/core/math_types.h"
#include "engine/renderer/material.h"
#include "engine/scene/scene.h"
#include "render_types.h"
#include <memory>
#include <string>
#include <vector>


namespace CHEngine
{
class Visuals
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void BeginToTexture(RenderTexture2D target);
    static void EndToTexture();

    static void DrawLine(Vector3 start, Vector3 end, Color color);
    static void DrawModel(const std::string &path, const Matrix &transform,
                          const std::vector<struct MaterialSlot> &overrides = {});
    static void DrawModel(std::shared_ptr<class ModelAsset> asset, const Matrix &transform,
                          const std::vector<struct MaterialSlot> &overrides = {});

    static void DrawScene(Scene *scene, const DebugRenderFlags *debugFlags = nullptr);

    static void SetDirectionalLight(Vector3 direction, Color color);
    static void SetAmbientLight(float intensity);

    static void DrawSkybox(const struct SkyboxComponent &skybox, const Camera3D &camera);
    static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                                float length, Color color);

    static void BeginUI();
    static void EndUI();
};
} // namespace CHEngine

#endif // CH_VISUALS_H
