#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/assert.h"
#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/shader_library.h"
#include "engine/scene/components/mesh_component.h"   // For MaterialSlot
#include "engine/scene/components/shader_component.h" // For ShaderUniform
#include "engine/scene/scene_settings.h"              // For GridSettings
#include "imgui.h"
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

struct RenderLight
{
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes (vec4 in GLSL)
    Vector3 position = {0, 0, 0};              // 12 bytes (vec3 in GLSL)
    float intensity = 1.0f;                    // 4 bytes (fits in vec3 alignment hole)
    Vector3 direction = {0, -1, 0};            // 12 bytes
    float radius = 10.0f;                      // 4 bytes
    float innerCutoff = 15.0f;                 // 4 bytes
    float outerCutoff = 20.0f;                 // 4 bytes
    int type = 0;                              // 4 bytes
    int enabled = 0;                           // 4 bytes
};
static_assert(sizeof(RenderLight) == 64, "RenderLight must be exactly 64 bytes for SSBO alignment");

struct RendererData
{
    Model SkyboxCube;

    std::unique_ptr<ShaderLibrary> Shaders;

    LightingSettings CurrentLighting;
    FogSettings CurrentFog;

    // Unified Lights for the scene (SSBO)
    static const int MaxLights = 256;
    RenderLight Lights[MaxLights];
    unsigned int LightSSBO = 0;
    bool LightsDirty = true;

    // Editor Icons
    Texture2D LightIcon = {0};
    Texture2D SpawnIcon = {0};
    Texture2D CameraIcon = {0};

    float DiagnosticMode = 0.0f; // 0: Normal, 1: Normals, 2: Lighting, 3: Albedo
    Vector3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
    Timestep Time = 0.0f;
};

struct DebugRenderFlags
{
    bool DrawColliders = false;
    bool DrawHierarchy = false;
    bool DrawCollisionModelBox = false;
    bool DrawGrid = false;
    bool DrawSelection = true;
    bool DrawLights = true;
    bool DrawSpawnZones = true;
};

class Renderer
{
public:
    static void Init();
    static void Shutdown();

    static bool IsInitialized()
    {
        return s_Instance != nullptr;
    }

    Renderer();
    ~Renderer();

    void BeginScene(const Camera3D& camera);
    void EndScene();

    void Clear(Color color);
    void SetViewport(int x, int y, int width, int height);

    void DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform = MatrixIdentity(),
                   const std::vector<MaterialSlot>& materialSlotOverrides = {}, int animationIndex = 0,
                   float frameIndex = 0.0f, int targetAnimationIndex = -1, float targetFrameIndex = 0.0f,
                   float blendWeight = 0.0f, const std::shared_ptr<ShaderAsset>& shaderOverride = nullptr,
                   const std::vector<ShaderUniform>& shaderUniformOverrides = {});
    void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
    void DrawGrid(int sliceCount, float spacing);
    void DrawModelInstanced(const std::shared_ptr<ModelAsset>& modelAsset, const std::vector<Matrix>& transforms,
                             const std::vector<MaterialSlot>& materialSlotOverrides);
    void DrawSkybox(const SkyboxSettings& settings, const Camera3D& camera);
    void DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color tint);
    void DrawCubeWires(const Matrix& transform, Vector3 size, Color color);
    void DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color);
    void DrawSphereWires(const Matrix& transform, float radius, Color color);

    void SetDirectionalLight(Vector3 direction, Color color);
    void SetAmbientLight(float intensity);
    void SetLight(int index, const RenderLight& light);
    void ClearLights();
    void ApplyEnvironment(const EnvironmentSettings& settings);
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

    static Renderer& Get()
    {
        CH_CORE_ASSERT(s_Instance, "Renderer instance is null!");
        return *s_Instance;
    }

private:
    void ApplyFogUniforms(ShaderAsset* shader);
    void EnsureShadersLoaded();
    void InitializeSkybox();

    // DrawModel decomposition helpers
    std::vector<Matrix> ComputeBoneMatrices(const std::shared_ptr<ModelAsset>& modelAsset, int animationIndex,
                                            float frameIndex, int targetAnimationIndex = -1,
                                            float targetFrameIndex = 0.0f, float blendWeight = 0.0f);
    Material ResolveMaterialForMesh(int meshIndex, const Model& model,
                                    const std::vector<MaterialSlot>& materialSlotOverrides);
    void BindShaderUniforms(ShaderAsset* shader, const std::vector<Matrix>& boneMatrices,
                            const std::vector<ShaderUniform>& shaderUniformOverrides);
    void BindMaterialUniforms(ShaderAsset* shader, const Material& material, int meshIndex, const Model& model,
                              const std::vector<MaterialSlot>& materialSlotOverrides);

private:
    static Renderer* s_Instance;
    std::unique_ptr<RendererData> m_Data;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
