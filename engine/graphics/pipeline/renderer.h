#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/core/engine_module.h"
#include "engine/common/timestep.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/camera_types.h"
#include "engine/graphics/pipeline/renderer_data.h"
#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/graphics/pipeline/lighting_manager.h"
#include "engine/graphics/pipeline/frame_manager.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Chained
{

class ShaderAsset;

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
    void LoadEngineResources();
    void InitializeResources();

    /// @brief Begin a new render frame with the given camera.
    void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip = 10000.0f);

    /// @brief Flush pending draw calls and finalize the frame.
    void EndScene();

    void Clear(const glm::vec4& color);
    void SetViewport(int x, int y, int width, int height);

    // Low-level Draw calls
    void DrawMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform);
    void DrawMeshInstanced(const Mesh& mesh, const Material& material, const std::vector<glm::mat4>& transforms);
    void DrawSkybox(uint32_t textureId, int skyboxMode, bool isHDR, float exposure, float brightness, float contrast,
                    const Camera3D& camera, bool flipped = false);
    void DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& position, float size,
                       const glm::vec4& tint);
    void DrawSprite(uint32_t textureId, const glm::mat4& transform, const glm::vec4& tint, bool flipX = false,
                    bool flipY = false);
    void ApplyPostProcessing(uint32_t screenTextureId, uint32_t depthTextureId, const Camera3D& camera,
                             ShaderAsset* overrideShader = nullptr, const std::vector<ShaderUniform>& uniforms = {});

    // Delegated to LightingManager
    void SetLight(int index, const RenderLight& light)    { m_Lighting->SetLight(index, light); }
    void SetLightCount(int count)                          { m_Lighting->SetLightCount(count); }
    void ClearLights()                                     { m_Lighting->ClearLights(); }
    void ApplyEnvironment(const EnvironmentSettings& s)    { m_Lighting->ApplyEnvironment(s); }
    void SetMainLight(const LightingSettings& s)           { m_Lighting->SetMainLight(s); }
    void SetShadowState(bool e, uint32_t id, const glm::mat4& m, float b) { m_Lighting->SetShadowState(e, id, m, b); }
    void SetLightingUniforms(ShaderAsset* shaderAsset)     { m_Lighting->SetLightingUniforms(shaderAsset, m_Frame->GetData()); }

    // Delegated to FrameManager
    void SetDiagnosticMode(float mode)                     { m_Frame->SetDiagnosticMode(mode); }
    void UpdateTime(Timestep time)                         { m_Frame->SetTime(time); }

    ShaderStorage& GetShaderLibrary() { return *m_Data->Shaders; }
    LightingManager& GetLighting()    { return *m_Lighting; }
    FrameManager&    GetFrame()       { return *m_Frame; }
    RendererData&    GetData()        { return *m_Data; }

    void     SetHeadless(bool headless)                       { m_Headless = headless; }
    void     SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }
    uint32_t GetViewportWidth()  const { return m_ViewportWidth; }
    uint32_t GetViewportHeight() const { return m_ViewportHeight; }
    bool     IsHeadless()        const { return m_Headless; }

protected:
    virtual void Initialize() override;
    virtual void Shutdown()   override;
    void Update(Timestep ts);

private:
    void InitializeSkybox();
    void CleanupSkybox();
    ShaderAsset* BindShader(const std::string& name);

private:
    std::unique_ptr<RendererData>    m_Data;
    std::unique_ptr<LightingManager> m_Lighting;
    std::unique_ptr<FrameManager>    m_Frame;
    bool     m_Headless       = false;
    uint32_t m_ViewportWidth  = 1280;
    uint32_t m_ViewportHeight = 720;
};

} // namespace Chained

#endif // CH_RENDERER_H