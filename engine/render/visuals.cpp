#include "visuals.h"
#include "api_context.h"
#include "draw_command.h"
#include "model_asset.h"
#include "scene_pipeline.h"


namespace CHEngine
{
void Visuals::Init()
{
    APIContext::Init();
}
void Visuals::Shutdown()
{
    APIContext::Shutdown();
}

void Visuals::BeginScene(const Camera3D &camera)
{
    BeginMode3D(camera);
}
void Visuals::EndScene()
{
    EndMode3D();
}

void Visuals::BeginToTexture(RenderTexture2D target)
{
    BeginTextureMode(target);
}
void Visuals::EndToTexture()
{
    EndTextureMode();
}

void Visuals::DrawLine(Vector3 start, Vector3 end, Color color)
{
    DrawCommand::DrawLine(start, end, color);
}

void Visuals::DrawModel(const std::string &path, const Matrix &transform,
                        const std::vector<MaterialSlot> &overrides)
{
    DrawCommand::DrawModel(path, transform, overrides);
}

void Visuals::DrawModel(std::shared_ptr<ModelAsset> asset, const Matrix &transform,
                        const std::vector<MaterialSlot> &overrides)
{
    if (asset)
        DrawCommand::DrawModel(asset->GetPath(), transform, overrides);
}

void Visuals::DrawScene(Scene *scene, const DebugRenderFlags *debugFlags)
{
    ScenePipeline::RenderScene(scene, debugFlags);
}

void Visuals::SetDirectionalLight(Vector3 direction, Color color)
{
    APIContext::SetDirectionalLight(direction, color);
}
void Visuals::SetAmbientLight(float intensity)
{
    APIContext::SetAmbientLight(intensity);
}

void Visuals::DrawSkybox(const SkyboxComponent &skybox, const Camera3D &camera)
{
    DrawCommand::DrawSkybox(skybox, camera);
}
void Visuals::DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height,
                              float length, Color color)
{
    DrawCommand::DrawCubeTexture(texture, position, width, height, length, color);
}

void Visuals::BeginUI()
{
}
void Visuals::EndUI()
{
}
} // namespace CHEngine
