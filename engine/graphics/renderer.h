#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/assert.h"
#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/graphics_types.h"
#include "engine/graphics/shader_library.h"
#include "raylib.h"
#include "raymath.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset;
class ShaderAsset;
class Entity; // Forward declaration for UIRenderer helpers if needed
class Scene;  // Forward declaration for UIRenderer helpers if needed

struct RendererData
{
    std::unique_ptr<ShaderLibrary> Shaders;

    float DiagnosticMode = 0.0f; // 0: Normal, 1: Normals, 2: Lighting, 3: Albedo
    Timestep Time = 0.0f;

    // Cache
    std::unordered_map<std::string, Texture2D> TextureCache;
};

class Renderer
{
public:
    static void LoadEngineResources(class AssetManager& assetManager);

    static bool IsInitialized();

public:
    static Renderer& Get();

    static void Init();
    static void Shutdown();

    Texture2D GetOrLoadTexture(const std::string& path);

private:
    Renderer();
    ~Renderer();

public:
    void BeginScene(const Camera3D& camera);
    void EndScene();

    void Clear(Color color);
    void SetViewport(int x, int y, int width, int height);

    void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
    void DrawGrid(int sliceCount, float spacing);
    void DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color tint);
    void DrawCubeWires(const Matrix& transform, Vector3 size, Color color);
    void DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color);
    void DrawSphereWires(const Matrix& transform, float radius, Color color);
    void ApplyPostProcessing(RenderTexture2D target, const Camera3D& camera);

    void SetDiagnosticMode(float mode);
    void UpdateTime(Timestep time);

    inline RendererData& GetData()
    {
        return *m_Data;
    }
    inline const RendererData& GetData() const
    {
        return *m_Data;
    }

    inline ShaderLibrary& GetShaderLibrary()
    {
        return *m_Data->Shaders;
    }

private:
    static Renderer* s_Instance;

    std::unique_ptr<RendererData> m_Data;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
