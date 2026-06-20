#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/foundation/base.h"

#include "engine/foundation/timestep.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/pipeline/shader_library.h"
#include "engine/scene/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/api/storage_buffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

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
class Renderer
{
public:
    static Renderer& Get() { return *s_Instance; }

    static void Init(bool headless = false);
    static void Shutdown();

    static void LoadEngineResources();

    static void InitializeResources();
    static void CleanupResources();

    static void BeginScene(const Camera3D& camera, float nearClip = 0.01f, float farClip = 10000.0f);
    static void EndScene();

    static void Clear(const glm::vec4& color);
    static void SetViewport(int x, int y, int width, int height);

    static void SetDiagnosticMode(float mode);
    static void UpdateTime(Timestep time);

    static ShaderLibrary& GetShaderLibrary();
    static RendererData& GetData();
    static UIRenderer* GetUIRenderer();

    static void SetHeadless(bool headless);
    static void SetViewportSize(uint32_t width, uint32_t height);
    
    static uint32_t GetViewportWidth();
    static uint32_t GetViewportHeight();
    static bool IsHeadless();

    static void Update(Timestep ts);

private:
   static Renderer* s_Instance;
};
} // namespace Chained

#endif // CH_RENDERER_H
