#include "renderer.h"
#include "core/log.h"
#include "renderer_api.h"
#include "shader.h"
#include "vertex_array.h"
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlgl.h>


namespace CHEngine
{
struct RendererData
{
    Camera3D SceneCamera;
    std::unique_ptr<RendererAPI> API;
    Color BackgroundColor = BLACK;
    bool CollisionDebugVisible = false;
    bool DebugInfoVisible = false;
};

static RendererData *s_Data = nullptr;

void Renderer::Init()
{
    s_Data = new RendererData();
    s_Data->SceneCamera = {.position = {0.0f, 10.0f, 10.0f},
                           .target = {0.0f, 0.0f, 0.0f},
                           .up = {0.0f, 1.0f, 0.0f},
                           .fovy = 45.0f,
                           .projection = CAMERA_PERSPECTIVE};

    s_Data->API = RendererAPI::Create();
    s_Data->API->Init();

    CD_CORE_INFO("Renderer Initialized");
}

bool Renderer::IsInitialized()
{
    return s_Data != nullptr;
}

void Renderer::Shutdown()
{
    delete s_Data;
    s_Data = nullptr;
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    if (s_Data && s_Data->API)
        s_Data->API->SetViewport(0, 0, width, height);
}

void Renderer::BeginScene(const Camera3D &camera)
{
    s_Data->SceneCamera = camera;
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const std::shared_ptr<Shader> &shader,
                      const std::shared_ptr<VertexArray> &vertexArray, const Matrix transform)
{
    shader->Bind();

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix projection;
    if (s_Data->SceneCamera.projection == CAMERA_PERSPECTIVE)
    {
        projection = MatrixPerspective(s_Data->SceneCamera.fovy * DEG2RAD, aspect, 0.01f, 1000.0f);
    }
    else
    {
        float top = s_Data->SceneCamera.fovy / 2.0f;
        float right = top * aspect;
        projection = MatrixOrtho(-right, right, -top, top, 0.01f, 1000.0f);
    }

    Matrix view = GetCameraMatrix(s_Data->SceneCamera);
    shader->SetMat4("u_ViewProjection", MatrixMultiply(view, projection));
    shader->SetMat4("u_Transform", transform);

    vertexArray->Bind();
    s_Data->API->DrawIndexed(vertexArray->GetIndexBuffer()->GetCount());
}

void Renderer::BeginMode3D(const Camera3D &camera)
{
    s_Data->SceneCamera = camera;
    ::BeginMode3D(camera);
}

void Renderer::BeginMode3D()
{
    ::BeginMode3D(s_Data->SceneCamera);
}

void Renderer::EndMode3D()
{
    ::EndMode3D();
}

void Renderer::BeginMode2D(const Camera2D &camera)
{
    ::BeginMode2D(camera);
}

void Renderer::EndMode2D()
{
    ::EndMode2D();
}

void Renderer::DrawModel(Model model, Vector3 position, Color tint)
{
    ::DrawModel(model, position, 1.0f, tint);
}

void Renderer::DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle,
                           Vector3 scale, Color tint)
{
    ::DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
}

Camera3D Renderer::GetCamera()
{
    return s_Data->SceneCamera;
}

void Renderer::SetCamera(const Camera3D &camera)
{
    s_Data->SceneCamera = camera;
}

void Renderer::SetBackgroundColor(Color color)
{
    s_Data->BackgroundColor = color;
}

Color Renderer::GetBackgroundColor()
{
    return s_Data->BackgroundColor;
}

void Renderer::SetCollisionDebugVisible(bool visible)
{
    s_Data->CollisionDebugVisible = visible;
}

bool Renderer::IsCollisionDebugVisible()
{
    return s_Data->CollisionDebugVisible;
}

void Renderer::SetDebugInfoVisible(bool visible)
{
    s_Data->DebugInfoVisible = visible;
}

bool Renderer::IsDebugInfoVisible()
{
    return s_Data->DebugInfoVisible;
}

} // namespace CHEngine
