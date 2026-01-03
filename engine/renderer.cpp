#include "renderer.h"

namespace CH
{
void Renderer::Init()
{
    // Any specific renderer init logic (shaders, etc) will go here
}

void Renderer::Shutdown()
{
}

void Renderer::BeginScene(const Camera3D &camera)
{
    BeginMode3D(camera);
}

void Renderer::EndScene()
{
    EndMode3D();
}

void Renderer::DrawGrid(int slices, float spacing)
{
    ::DrawGrid(slices, spacing);
}

void Renderer::DrawLine(Vector3 start, Vector3 end, Color color)
{
    ::DrawLine3D(start, end, color);
}

void Renderer::BeginUI()
{
    // Currently BeginDrawing is handled by Application,
    // but we might want custom UI state here.
}

void Renderer::EndUI()
{
}
} // namespace CH
