#ifndef RENDERER_H
#define RENDERER_H

#include <raylib.h>

namespace CHEngine
{

/**
 * @brief Static Renderer API - Hazel-style interface for rendering
 *
 * Provides a clean, static interface for rendering operations.
 * Replaces verbose Engine::Instance().GetRenderManager() calls.
 *
 * Example usage:
 *   Renderer::BeginScene(camera);
 *   Renderer::DrawModel(model, transform);
 *   Renderer::EndScene();
 */
class Renderer
{
public:
    // Scene Management
    static void BeginScene(const Camera3D &camera);
    static void EndScene();

    // 2D Rendering
    static void BeginMode2D();
    static void EndMode2D();

    // Drawing
    static void DrawModel(Model &model, Vector3 position, float scale, Color tint);
    static void DrawModelEx(Model &model, Vector3 position, Vector3 rotationAxis,
                            float rotationAngle, Vector3 scale, Color tint);

    // Camera Access
    static Camera3D &GetCamera();
    static void SetCamera(const Camera3D &camera);

    // Clear
    static void Clear(Color color);

    // Internal - called by Application
    static void Init();
    static void Shutdown();

private:
    Renderer() = delete; // Static class, no instances
};

} // namespace CHEngine

#endif // RENDERER_H
