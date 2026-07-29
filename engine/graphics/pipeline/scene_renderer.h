#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/scene/scene_settings.h"
#include "engine/core/profiler.h"
#include "engine/scene/entity.h"
#include "engine/graphics/pipeline/render_pass.h"
#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "engine/scene/components/model_component.h"
#include "engine/assets/types/model_asset.h"
#include "engine/graphics/api/texture.h"
#include "engine/scene/components/animation_component.h"

namespace Chained
{
struct Frustum;
class AssetManager;
class Renderer; // forward declaration — full include in .cpp only
class Shader;   // forward declaration — full include in .cpp only

enum class RenderPassStage
{
    Opaque,
    Transparent,
    Both
};

// Per-frame lighting state shared by scene rendering and post-processing.

// Per-render-pass toggles for runtime and editor scene rendering.
struct SceneRenderOptions
{
    std::shared_ptr<EnvironmentAsset> EnvironmentOverride = nullptr;

    bool ShowDebugColliders = false;
    bool ShowDebugCollisionModelBox = false;
    bool ShowDebugSpawnZones = false;
    bool DrawGrid = false;
    int SetCollisionWireframeMode = 0;
};

struct AnimatedEntry
{
    ModelAsset* Asset;
    glm::mat4 WorldTransform;
    Shader* ShaderOverride;
    std::vector<ShaderUniform> CustomUniforms;
    AnimationComponent Animation;
};

struct RenderItem
{
    ModelAsset* Asset;
    glm::mat4 Transform;
    std::vector<glm::mat4> BoneMatrices;
    std::vector<Material> Materials;
    Shader* ShaderOverride;
    std::vector<ShaderUniform> CustomUniforms;
    float Distance = 0.0f;
};

// High-level scene render orchestrator that collects visible entities, draws them, and emits debug overlays.
class SceneRenderer
{
public:
    SceneRenderer();
    ~SceneRenderer() = default;

    // Renders the scene using the supplied camera and options.
    // Internally manages Renderer::BeginScene/EndScene — callers should NOT call them separately.
    void RenderScene(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera, float nearClip,
                     float farClip, const SceneRenderOptions& options);

    // Architectural Helper: Retrieves the primary camera from scene entities.
    static std::optional<Camera3D> GetActiveCamera(entt::registry& registry);
    static Entity GetPrimaryCameraEntity(entt::registry& registry, entt::registry* registryPtr);

    void AddPass(std::unique_ptr<IRenderPass> pass);

    // Core Render Pass API
    void PrepareLights(entt::registry& registry, const Frustum& frustum);
    void CollectAndRenderItems(entt::registry& registry, const Frustum& frustum, const glm::vec3& cameraPos);

    // Shared "ready ModelAsset -> RenderItem" path used for both ModelComponent and
    // PrimitiveComponent: frustum-culls, applies an optional ShaderComponent override,
    // splits opaque/transparent, and pushes to the queues. Returns true if queued.
    bool EnqueueModelAsset(entt::registry& registry, entt::entity entity, ModelAsset* modelAsset,
                           const glm::mat4& worldTransform, const Frustum& frustum, const glm::vec3& cameraPos);

    void DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries);
    void DrawModel(ModelAsset* modelAsset, const glm::mat4& transform, const std::vector<glm::mat4>& boneMatrices = {},
                   const std::vector<Material>& materials = {}, Shader* shaderOverride = nullptr,
                   const std::vector<ShaderUniform>& shaderUniformOverrides = {},
                   RenderPassStage pass = RenderPassStage::Both);

    Material ResolveMaterialForMesh(int meshIndex, const Model& model, const std::vector<Material>& materials = {},
                                    ModelAsset* modelAsset = nullptr);

    void BindShaderUniforms(Shader* shader, const std::vector<glm::mat4>& boneMatrices,
                            const std::vector<ShaderUniform>& shaderUniformOverrides);

    void BindMaterialUniforms(Shader* shader, const Material& material, int meshIndex, const Model& model);

    // Expose internals for passes
    std::vector<RenderItem>& GetOpaqueQueue()
    {
        return m_OpaqueQueue;
    }
    std::vector<RenderItem>& GetTransparentQueue()
    {
        return m_TransparentQueue;
    }
    EnvironmentSettings& GetEnvironment()
    {
        return m_CurrentEnv;
    }

    void RenderSprites(entt::registry& registry, const Camera3D& camera);
    void RenderDebug(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera,
                     const SceneRenderOptions& options);
    void DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options);

private:
    std::vector<std::unique_ptr<IRenderPass>> m_RenderPasses;

    ProfilerStats m_CurrentStats;
    std::vector<RenderItem> m_OpaqueQueue;
    std::vector<RenderItem> m_TransparentQueue;

    EnvironmentSettings m_CurrentEnv;

    // Skybox cache
    std::shared_ptr<Texture> m_CachedCubemap;
    std::string m_CachedCubemapPath;
    std::shared_ptr<ModelAsset> m_SkyboxCubeModel;
};

} // namespace Chained

#endif // CH_SCENE_RENDERER_H
