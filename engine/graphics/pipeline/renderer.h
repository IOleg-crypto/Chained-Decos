#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/timestep.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/pipeline/shader_library.h"
#include "raylib.h"
#include "raymath.h"
#include <memory>
#include <vector>

namespace CHEngine
{

// Low-level representation of a light for SSBO
struct RenderLight
{
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
    Vector3 position = {0, 0, 0};              // 12 bytes
    float intensity = 1.0f;                    // 4 bytes
    Vector3 direction = {0, -1, 0};            // 12 bytes
    float radius = 10.0f;                      // 4 bytes
    float innerCutoff = 15.0f;                 // 4 bytes
    float outerCutoff = 20.0f;                 // 4 bytes
    int type = 0;                              // 4 bytes
    int enabled = 0;                           // 4 bytes
};

struct LightingData
{
    static constexpr int MaxLights = 256;
    RenderLight Lights[MaxLights];
    unsigned int LightSSBO = 0;
    bool LightsDirty = true;

    LightingSettings CurrentLighting;
    FogSettings CurrentFog;
};

struct SkyboxData
{
    Model SkyboxCube;
    Material SkyboxMaterial;

    TextureCubemap CachedCubemap = { 0 };
    std::string CachedCubemapPath = "";
    unsigned int SourceTextureId = 0;
};

struct RendererData
{
    SkyboxData Skybox;
    LightingData Lighting;

    std::unique_ptr<ShaderLibrary> Shaders;

    float DiagnosticMode = 0.0f;
    Vector3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
    Timestep Time = 0.0f;
    int LightCount = 0;
    unsigned int CurrentShaderId = 0;
    EnvironmentSettings CurrentEnv;

    Matrix CurrentView = MatrixIdentity();
    Matrix CurrentProj = MatrixIdentity();
};

class Renderer
{
public:
    static void Init();
    static void Shutdown();
    static Renderer& Get();
    static void LoadEngineResources();

    Renderer();
    ~Renderer();

    void InternalInit();
    void InternalShutdown();

    void BeginScene(const Camera3D& camera);
    void EndScene();

    void Clear(Color color);
    void SetViewport(int x, int y, int width, int height);

    // Low-level Draw calls
    void DrawMesh(const Mesh& mesh, const Material& material, const Matrix& transform);
    void DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<Matrix>& transforms);
    void DrawMeshWire(const Mesh& mesh, Color color, const Matrix& transform);
    
    void DrawLine(Vector3 start, Vector3 end, Color color);
    void DrawGrid(int slices, float spacing);
    void DrawInfiniteGrid(const Camera3D& camera, float spacing, Color color);
    void DrawSkybox(const SkyboxSettings& settings, const Camera3D& camera);
    void DrawBillboard(const Camera3D& camera, Texture2D texture, Vector3 position, float size, Color tint);
    void DrawCubeWires(const Matrix& transform, Vector3 size, Color color);
    void DrawCapsuleWires(const Matrix& transform, float radius, float height, Color color);
    void DrawSphereWires(const Matrix& transform, float radius, Color color);

    void ApplyPostProcessing(RenderTexture2D screenTexture, const Camera3D& camera);

    // Light management
    void SetLight(int index, const RenderLight& light);
    void SetLightCount(int count);
    void ClearLights();
    void ApplyEnvironment(const EnvironmentSettings& settings);
    void SetMainLight(const LightingSettings& settings);
    void SetDiagnosticMode(float mode);
    void UpdateTime(Timestep time);

    ShaderLibrary& GetShaderLibrary() { return *m_Data->Shaders; }
    RendererData& GetData() { return *m_Data; }

    static bool IsInitialized();

private:
    void ApplyFogUniforms(Shader shader);
    void InitializeSkybox();
    void CleanupSkybox();
    TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format);

private:
    std::unique_ptr<RendererData> m_Data;

    static Renderer* s_Instance;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
