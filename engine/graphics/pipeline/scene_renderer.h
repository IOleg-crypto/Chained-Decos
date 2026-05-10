#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene_settings.h"
#include "engine/core/profiler.h"
#include "engine/scene/entity.h"
#include "entt/entt.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/primitive_component.h"

namespace CHEngine
{
struct Frustum;

enum class RenderPassStage
{
    Opaque,
    Transparent,
    Both
};

// Per-render-pass toggles for runtime and editor scene rendering.
struct SceneRenderOptions
{
    std::shared_ptr<class EnvironmentAsset> EnvironmentOverride = nullptr;

    bool ShowDebugColliders        = false;
    bool ShowDebugCollisionModelBox = false;
    bool ShowDebugSpawnZones       = true;
    bool DrawGrid                  = false;
    bool ShowEditorIcons           = true;
    int  SetCollisionWireframeMode = 0;
};

// Cached editor icon textures used during scene rendering.
struct EditorResourcesData
{
    unsigned int LightIconId   = 0;
    unsigned int SpawnIconId   = 0;
    unsigned int CameraIconId  = 0;
};

// High-level scene render orchestrator that collects visible entities, draws them, and emits debug overlays.
class SceneRenderer
{
public:
    SceneRenderer()  = default;
    ~SceneRenderer() = default;

    // Renders the scene using the supplied camera and options.
    void RenderScene(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera, float nearClip, float farClip,
                     const SceneRenderOptions& options);

    // Architectural Helper: Retrieves the primary camera from scene entities.
    static std::optional<Camera3D> GetActiveCamera(entt::registry& registry);
    static Entity GetPrimaryCameraEntity(entt::registry& registry, entt::registry* registryPtr);

private:
    struct AnimatedEntry
    {
        class ModelAsset*                  asset;
        glm::mat4                          worldTransform;
        std::vector<MaterialSlot>          materials;
        class ShaderAsset*                 shaderOverride;
        std::vector<ShaderUniform>         customUniforms;
        AnimationComponent                 animation;
    };

    struct RenderItem
    {
        ModelAsset*                  Asset;
        glm::mat4                    Transform;
        std::vector<MaterialSlot>    Materials;
        std::vector<glm::mat4>       BoneMatrices;
        ShaderAsset*                 ShaderOverride;
        std::vector<ShaderUniform>   CustomUniforms;
        float                        Distance = 0.0f;
    };

    // Render passes
    void RenderModels(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera, float nearClip, float farClip);
    void RenderSprites(entt::registry& registry, const Camera3D& camera);
    void RenderDebug(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera, const SceneRenderOptions& options);
    void RenderEditorIcons(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera);

    // Helpers
    void PrepareLights(entt::registry& registry, const Frustum& frustum);
    void CollectAndRenderItems(entt::registry& registry, const Frustum& frustum,
                               const glm::vec3& cameraPos);
    
    void DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries);

    void DrawModel(ModelAsset* modelAsset, const glm::mat4& transform,
                   const std::vector<MaterialSlot>& materialSlotOverrides = {},
                   const std::vector<glm::mat4>& boneMatrices = {},
                   ShaderAsset* shaderOverride = nullptr,
                   const std::vector<ShaderUniform>& shaderUniformOverrides = {},
                   RenderPassStage pass = RenderPassStage::Both);

    Material ResolveMaterialForMesh(int meshIndex, const Model& model,
                                   const std::vector<MaterialSlot>& materialSlotOverrides,
                                   ModelAsset* modelAsset = nullptr);

    void BindShaderUniforms(ShaderAsset* shader, const std::vector<glm::mat4>& boneMatrices,
                           const std::vector<ShaderUniform>& shaderUniformOverrides);

    void BindMaterialUniforms(ShaderAsset* shader, const Material& material, int meshIndex,
                             const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides);

    void DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options);
    void DrawCollisionModelBoxDebug(entt::registry& registry);
    void DrawSpawnDebug(entt::registry& registry);

    static BoundingBox CalculateColliderWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform);
    static glm::mat4 GetWorldTransform(entt::registry& registry, entt::entity entity);
    static glm::vec3 GetWorldPosition(entt::registry& registry, entt::entity entity);

private:
    EditorResourcesData m_EditorResources;
    ProfilerStats       m_CurrentStats;

    std::vector<RenderItem> m_OpaqueQueue;
    std::vector<RenderItem> m_TransparentQueue;
};

} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
