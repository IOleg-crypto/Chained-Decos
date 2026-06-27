#ifndef CH_RENDERER_H
#define CH_RENDERER_H

#include "engine/foundation/base.h"

#include "engine/foundation/timestep.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/graphics/camera_types.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/pipeline/renderer_data.h"
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

// RendererData structures are moved to renderer_types.h

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

    ShaderStorage& GetShaderStorage();
    RendererData& GetData();

    void SetHeadless(bool headless);
    void SetViewportSize(uint32_t width, uint32_t height);
    
    uint32_t GetViewportWidth();
    uint32_t GetViewportHeight();
    bool IsHeadless();

private:
    std::unique_ptr<RendererData> m_Data;
    bool m_Headless = false;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
};
} // namespace Chained

#endif // CH_RENDERER_H
