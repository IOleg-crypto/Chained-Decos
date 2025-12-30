#include "Renderer.h"
#include "core/Engine.h"

namespace CHEngine
{

// Internal data
struct RendererData
{
    Camera3D *ActiveCamera = nullptr;
};

static RendererData *s_Data = nullptr;

void Renderer::Init()
{
    s_Data = new RendererData();
}

void Renderer::Shutdown()
{
    delete s_Data;
    s_Data = nullptr;
}

void Renderer::BeginScene(const Camera3D &camera)
{
    // Delegate to RenderManager
    Engine::Instance().GetRenderManager().BeginMode3D(const_cast<Camera3D &>(camera));

    if (s_Data)
    {
        s_Data->ActiveCamera = const_cast<Camera3D *>(&camera);
    }
}

void Renderer::EndScene()
{
    Engine::Instance().GetRenderManager().EndMode3D();

    if (s_Data)
    {
        s_Data->ActiveCamera = nullptr;
    }
}

void Renderer::BeginMode2D()
{
    // For 2D rendering if needed
    BeginMode2D();
}

void Renderer::EndMode2D()
{
    ::EndMode2D();
}

void Renderer::DrawModel(Model &model, Vector3 position, float scale, Color tint)
{
    ::DrawModel(model, position, scale, tint);
}

void Renderer::DrawModelEx(Model &model, Vector3 position, Vector3 rotationAxis,
                           float rotationAngle, Vector3 scale, Color tint)
{
    ::DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
}

Camera3D &Renderer::GetCamera()
{
    return Engine::Instance().GetRenderManager().GetCamera();
}

void Renderer::SetCamera(const Camera3D &camera)
{
    Engine::Instance().GetRenderManager().GetCamera() = camera;
}

void Renderer::Clear(Color color)
{
    ClearBackground(color);
}

} // namespace CHEngine
