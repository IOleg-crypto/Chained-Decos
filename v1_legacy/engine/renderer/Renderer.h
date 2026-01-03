#ifndef CD_CORE_RENDERER_RENDERER_H
#define CD_CORE_RENDERER_RENDERER_H

#include "renderer_api.h"
#include "shader.h"
#include "vertex_array.h"
#include <memory>
#include <raylib.h>

namespace CHEngine
{

class Renderer
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static void OnWindowResize(uint32_t width, uint32_t height);

    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    static void Submit(const std::shared_ptr<Shader> &shader,
                       const std::shared_ptr<VertexArray> &vertexArray, const Matrix transform);

    static void BeginMode3D(const Camera3D &camera);
    static void BeginMode3D();
    static void EndMode3D();

    static void BeginMode2D(const Camera2D &camera);
    static void EndMode2D();

    static void DrawModel(Model model, Vector3 position, Color tint);
    static void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis,
                            float rotationAngle, Vector3 scale, Color tint);

    static Camera3D GetCamera();
    static void SetCamera(const Camera3D &camera);

    static void SetBackgroundColor(Color color);
    static Color GetBackgroundColor();

    static void SetCollisionDebugVisible(bool visible);
    static bool IsCollisionDebugVisible();

    static void SetDebugInfoVisible(bool visible);
    static bool IsDebugInfoVisible();

    static inline RendererAPI::API GetAPI()
    {
        return RendererAPI::GetAPI();
    }

private:
    Renderer() = delete;
};

} // namespace CHEngine

#endif // CD_CORE_RENDERER_RENDERER_H
