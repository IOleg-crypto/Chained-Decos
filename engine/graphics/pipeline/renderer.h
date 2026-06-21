#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/foundation/base.h"

#include "engine/foundation/timestep.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/pipeline/shader_library.h"
#include "engine/graphics/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/api/storage_buffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include "engine/core/engine_module.h"

namespace Chained
{
    class Texture;
    class TextureSystem;
    class UIRenderer;


// Packed light data uploaded to the renderer SSBO.
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

// Data structures for Uniform Buffers (UBOs)
struct CameraData
{
    glm::mat4 ViewProjection;
    glm::mat4 Projection;
    glm::mat4 View;
};

struct RendererData
{
    std::unique_ptr<ShaderLibrary> Shaders;

    std::shared_ptr<UniformBuffer> CameraUBO;
    std::shared_ptr<UniformBuffer> GlobalUBO;

    float DiagnosticMode = 0.0f;
    glm::vec3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
    Timestep Time = 0.0f;
    int LightCount = 0;
    unsigned int CurrentShaderId = 0;
    EnvironmentParameters CurrentEnv;

    glm::mat4 CurrentView = glm::mat4(1.0f);
    glm::mat4 CurrentProj = glm::mat4(1.0f);
    
    // Engine static resources
    std::shared_ptr<VertexArray> FullscreenQuadVAO;
    std::shared_ptr<VertexArray> BillboardVAO;
    std::shared_ptr<VertexArray> SpriteVAO;
    std::shared_ptr<VertexArray> GridPlaneVAO;
};

// Singleton renderer facade that owns GPU resources, frame state, and low-level draw calls.
class Renderer : public EngineModule
{
public:
    Renderer(bool headless = false);
    ~Renderer() override;

    void Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    void LoadEngineResources();

    void InitializeResources();
    void CleanupResources();

    void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip = 10000.0f);
    void EndScene();

    void Clear(const glm::vec4& color);
    void SetViewport(int x, int y, int width, int height);

    void SetDiagnosticMode(float mode);
    void UpdateTime(Timestep time);

    ShaderLibrary& GetShaderLibrary();
    RendererData& GetData();
    UIRenderer* GetUIRenderer();

    void SetHeadless(bool headless);
    void SetViewportSize(uint32_t width, uint32_t height);
    
    uint32_t GetViewportWidth();
    uint32_t GetViewportHeight();
    bool IsHeadless();

private:
    std::unique_ptr<RendererData> m_Data;
    std::unique_ptr<UIRenderer> m_UI;
    bool m_Headless = false;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
};
} // namespace Chained

#endif // CH_RENDERER_H
