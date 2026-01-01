#ifndef RENDERER_H
#define RENDERER_H

#include "RendererAPI.h"
#include "Shader.h"
#include "VertexArray.h"
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

    static inline RendererAPI::API GetAPI()
    {
        return RendererAPI::GetAPI();
    }

private:
    Renderer() = delete;
};

} // namespace CHEngine

#endif // RENDERER_H
