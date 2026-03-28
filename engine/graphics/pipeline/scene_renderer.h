#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/core/profiler.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct Camera3D;
struct Texture; // Raylib's Texture2D is a typedef of Texture
struct Mesh;
struct Material;
struct Model;
struct Color;
struct BoundingBox;

namespace CHEngine
{
struct Frustum;

struct SceneRenderOptions
{
    float TargetFPS = 60.0f;
    std::shared_ptr<class EnvironmentAsset> EnvironmentOverride = nullptr;
    
    // Debug Rendering Flags
    bool ShowDebugColliders = false;
    bool ShowDebugCollisionModelBox = false;
    bool ShowDebugSpawnZones = true;
    bool DrawGrid = false;
    bool ShowEditorIcons = true;
};

struct EditorResourcesData
{
    unsigned int LightIconId = 0;
    unsigned int SpawnIconId = 0;
    unsigned int CameraIconId = 0;
};

class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

public:
    void RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                     const SceneRenderOptions& options);


    // Internal methods used by RenderScene
    void RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                      const SceneRenderOptions& options);
    void RenderDebug(Scene* scene, const Camera3D& camera, const SceneRenderOptions& options);
    void RenderEditorIcons(Scene* scene, const Camera3D& camera);
    void RenderSprites(Scene* scene);

private:
    struct AnimatedEntry
    {
        std::shared_ptr<class ModelAsset> asset;
        glm::mat4 worldTransform;
        std::vector<MaterialSlot> materials;
        std::shared_ptr<class ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        AnimationComponent animation;
    };

    struct InstanceKey
    {
        size_t Hash = 0;
        InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats);
        bool operator<(const InstanceKey& o) const { return Hash < o.Hash; }
        bool operator==(const InstanceKey& o) const { return Hash == o.Hash; }
    };

    struct InstanceKeyHash
    {
        size_t operator()(const InstanceKey& k) const { return k.Hash; }
    };

    struct InstanceGroup
    {
        std::shared_ptr<class ModelAsset> asset;
        std::vector<glm::mat4> transforms;
        std::vector<MaterialSlot> materials;
    };

private:
    void PrepareLights(entt::registry& registry, const Frustum& frustum);
    void CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                            std::vector<AnimatedEntry>& animatedEntries,
                            std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);
    void DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries, const SceneRenderOptions& options);
    void DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);

    void DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const glm::mat4& transform,
                   const std::vector<MaterialSlot>& materialSlotOverrides = {},
                   const std::vector<glm::mat4>& boneMatrices = {},
                   const std::shared_ptr<ShaderAsset>& shaderOverride = nullptr,
                   const std::vector<ShaderUniform>& shaderUniformOverrides = {});

    Material ResolveMaterialForMesh(int meshIndex, const struct Model& model,
                                    const std::vector<MaterialSlot>& materialSlotOverrides);

    void BindShaderUniforms(ShaderAsset* shader, const std::vector<glm::mat4>& boneMatrices,
                            const std::vector<ShaderUniform>& shaderUniformOverrides);

    void BindMaterialUniforms(ShaderAsset* shader, const struct Material& material, int meshIndex,
                              const struct Model& model, const std::vector<MaterialSlot>& materialSlotOverrides);

    void DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options);
    void DrawCollisionModelBoxDebug(entt::registry& registry, const SceneRenderOptions& options);
    void DrawSpawnDebug(entt::registry& registry, const SceneRenderOptions& options);

    static struct BoundingBox CalculateColliderWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform);

    static glm::mat4 GetWorldTransform(entt::registry& registry, entt::entity entity);
    static glm::vec3 GetWorldPosition(entt::registry& registry, entt::entity entity);

private:
    EditorResourcesData m_EditorResources;
    ProfilerStats m_CurrentStats;
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
