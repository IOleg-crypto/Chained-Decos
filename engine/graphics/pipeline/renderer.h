#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/timestep.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/pipeline/shader_library.h"
#include "engine/graphics/api/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace CHEngine
{

// Low-level representation of a light for SSBO
struct RenderLight
{
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
    glm::vec3 position = {0, 0, 0};              // 12 bytes
    float intensity = 1.0f;                  // 4 bytes
    glm::vec3 direction = {0, -1, 0};            // 12 bytes
    float radius = 10.0f;                     // 4 bytes
    float innerCutoff = 15.0f;                // 4 bytes
    float outerCutoff = 20.0f;                // 4 bytes
    int type = 0;                               // 4 bytes
    int enabled = 0;                            // 4 bytes
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
    std::unique_ptr<Model> SkyboxCubeModel;
    std::unique_ptr<Model> SkyboxSphereModel;

    unsigned int CachedCubemapId = 0;
    std::string CachedCubemapPath = "";
    unsigned int SourceTextureId = 0;
};

struct RendererData
{
    SkyboxData Skybox;
    LightingData Lighting;

    std::unique_ptr<ShaderLibrary> Shaders;

    float DiagnosticMode = 0.0f;
    glm::vec3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
    Timestep Time = 0.0f;
    int LightCount = 0;
    unsigned int CurrentShaderId = 0;
    EnvironmentSettings CurrentEnv;

    glm::mat4 CurrentView = glm::mat4(1.0f);
    glm::mat4 CurrentProj = glm::mat4(1.0f);
    
    // Engine static resources
    std::shared_ptr<VertexArray> FullscreenQuadVAO;
    std::shared_ptr<VertexArray> BillboardVAO;
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

    void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip = 1000.0f);
    void EndScene();

    void Clear(const glm::vec4& color);
    void SetViewport(int x, int y, int width, int height);

    // Low-level Draw calls
    void DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
    void DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms);
    void DrawMeshWire(const Mesh& mesh, const glm::vec4& color, const glm::mat4& transform);
    
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    void DrawGrid(int slices, float spacing);
    void DrawInfiniteGrid(const Camera3D& camera, float spacing, const glm::vec4& color);
    void DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float contrast, const Camera3D& camera);
    void DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size, const glm::vec4& tint);
    void DrawCubeWires(const glm::mat4& transform, const glm::vec3& size, const glm::vec4& color);
    void DrawCapsuleWires(const glm::mat4& transform, float radius, float height, const glm::vec4& color);
    void DrawSphereWires(const glm::mat4& transform, float radius, const glm::vec4& color);

    void ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera);

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
    void ApplyFogUniforms(const std::shared_ptr<ShaderAsset>& shader);
    void InitializeSkybox();
    void CleanupSkybox();

private:
    std::unique_ptr<RendererData> m_Data;

    static Renderer* s_Instance;
};
} // namespace CHEngine

#endif // CH_RENDERER_H
