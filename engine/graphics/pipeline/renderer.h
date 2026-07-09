#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/assets/types/environment_asset.h"
#include "engine/common/timestep.h"
#include "engine/core/engine_module.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/pipeline/shader_storage.h"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Chained
{
class Texture;

// Packed light data uploaded to the renderer SSBO.
struct RenderLight
{
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
    glm::vec3 position = {0, 0, 0};             // 12 bytes
    float intensity = 1.0f;                     // 4 bytes
    glm::vec3 direction = {0, -1, 0};           // 12 bytes
    float radius = 10.0f;                       // 4 bytes
    float innerCutoff = 15.0f;                  // 4 bytes
    float outerCutoff = 20.0f;                  // 4 bytes
    int type = 0;                               // 4 bytes
    int enabled = 0;                            // 4 bytes
};

// Shared mesh resources for primitive and debug rendering.
struct StaticResources
{
    std::unique_ptr<Model> UnitCubeModel;
    std::unique_ptr<Model> UnitSphereModel;
    std::unique_ptr<Model> UnitCapsuleModel;
    std::unique_ptr<Model> WireCubeModel; // 12-edge wireframe cube for GL_LINES
};

// Data structures for Uniform Buffers (UBOs)
struct CameraData
{
    glm::mat4 ViewProjection;
    glm::mat4 Projection;
    glm::mat4 View;
};

// Cached skybox resources and the last uploaded environment texture.
struct SkyboxData
{
    std::unique_ptr<Model> SkyboxCubeModel;
    std::unique_ptr<Model> SkyboxSphereModel;

    std::shared_ptr<Texture> CachedCubemap;
    std::string CachedCubemapPath = "";
    unsigned int SourceTextureId = 0;
};

struct LineVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
};

struct LightingData
{
    static constexpr int MaxLights = 256;
    RenderLight Lights[MaxLights];
    std::shared_ptr<StorageBuffer> LightSSBO;
    bool LightsDirty = true;
    int LightCount = 0;
    LightingSettings CurrentLighting;
    FogSettings CurrentFog;
};

// Mutable renderer state owned by the singleton renderer.
struct RendererData
{
    SkyboxData Skybox;
    StaticResources Resources;
    LightingData Lighting;

    std::unique_ptr<ShaderStorage> Shaders;

    std::shared_ptr<UniformBuffer> CameraUBO;
    std::shared_ptr<UniformBuffer> GlobalUBO;

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
    std::shared_ptr<VertexArray> QuadVAO;
    std::shared_ptr<VertexArray> GridPlaneVAO;

    // Instancing cache
    std::shared_ptr<class VertexBuffer> InstanceBuffer;
    uint32_t InstanceBufferCapacity = 0;
    std::unordered_map<VertexArray*, std::shared_ptr<VertexArray>> InstancedVAOCache;

    // Line rendering cache
    std::shared_ptr<class VertexBuffer> LineVBO;
    std::shared_ptr<VertexArray> LineVAO;
    std::vector<LineVertex> LineVertexBuffer;
    uint32_t LineVBOSize = 0;
};

/// @brief Singleton renderer facade that owns GPU resources, frame state, and low-level draw calls.
///
/// Manages the OpenGL rendering pipeline: shader compilation, uniform buffer management,
/// draw call batching, skybox rendering, and post-processing. Accessed globally via ServiceLocator.
class CH_API Renderer : public EngineModule
{
public:
    Renderer();
    virtual ~Renderer() override;

    /// @brief Load engine-level shaders and static resources (unit meshes, fullscreen quad).
    /// Called once after the renderer is registered in ServiceLocator.
    void LoadEngineResources();

    void InitializeResources();
    void CleanupResources();

    /// @brief Begin a new render frame with the given camera.
    /// Uploads camera matrices to the UBO and prepares internal state for draw calls.
    /// @param camera The camera to render from.
    /// @param nearClip Near clipping plane distance.
    /// @param farClip Far clipping plane distance.
    void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip);

    /// @brief Flush pending draw calls and finalize the frame.
    void EndScene();

    void Clear(const glm::vec4& color);
    void SetViewport(int x, int y, int width, int height);

    // Low-level Draw calls
    /// @brief Submit a single mesh for rendering with the given material and transform.
    void DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);

    /// @brief Submit multiple instances of the same mesh in a single draw call.
    void DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms);

    void DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float contrast,
                    const Camera3D& camera, bool flipped = false);
    void DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size,
                       const glm::vec4& tint);
    void DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX = false,
                    bool flipY = false);

    void ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera,
                             ShaderAsset* overrideShader = nullptr, const std::vector<ShaderUniform>& uniforms = {});

    // Light management
    void SetLight(int index, const RenderLight& light);
    void SetLightCount(int count);
    void ClearLights();
    void ApplyEnvironment(const EnvironmentSettings& settings);
    void SetMainLight(const LightingSettings& settings);
    void SetDiagnosticMode(float mode);
    void UpdateTime(Timestep time);

    ShaderStorage& GetShaderLibrary()
    {
        return *m_Data->Shaders;
    }
    RendererData& GetData()
    {
        return *m_Data;
    }

    void SetHeadless(bool headless)
    {
        m_Headless = headless;
    }
    void SetViewportSize(uint32_t width, uint32_t height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }

    uint32_t GetViewportWidth() const
    {
        return m_ViewportWidth;
    }
    uint32_t GetViewportHeight() const
    {
        return m_ViewportHeight;
    }
    bool IsHeadless() const
    {
        return m_Headless;
    }

protected:
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Update(Timestep ts) override;

private:
    void ApplyFogUniforms(ShaderAsset* shaderAsset);
    void InitializeSkybox();
    void CleanupSkybox();

public:
    void SetLightingUniforms(ShaderAsset* shaderAsset);

private:
    std::unique_ptr<RendererData> m_Data;
    bool m_Headless = false;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
};
} // namespace Chained

#endif // CH_RENDERER_H