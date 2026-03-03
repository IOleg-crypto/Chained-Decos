#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/graphics/renderer.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/scene/scene.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
struct Frustum;

class SceneRenderer
{
public:
    static SceneRenderer& Get();
    static void Init();
    static void Shutdown();

    bool IsInitialized() const
    {
        return s_Instance != nullptr;
    }

public:
    void Clear(Scene* scene);
    void RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                     const DebugRenderFlags* debugFlags = nullptr);

    // Internal methods used by RenderScene
    void RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep);
    void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
    void RenderEditorIcons(Scene* scene, const Camera3D& camera);
    void RenderSprites(Scene* scene);

    // Environment & Lighting
    void SetLight(int index, const RenderLight& light);
    void SetLightCount(int count);
    void ClearLights();

    void ApplyEnvironment(const EnvironmentSettings& settings);

    void DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform = MatrixIdentity(),
                   const std::vector<MaterialSlot>& materialSlotOverrides = {}, int animationIndex = 0,
                   float frameIndex = 0.0f, int targetAnimationIndex = -1, float targetFrameIndex = 0.0f,
                   float blendWeight = 0.0f, const std::shared_ptr<ShaderAsset>& shaderOverride = nullptr,
                   const std::vector<ShaderUniform>& shaderUniformOverrides = {});

    void DrawModelInstanced(const std::shared_ptr<ModelAsset>& modelAsset, const std::vector<Matrix>& transforms,
                            const std::vector<MaterialSlot>& materialSlotOverrides);

    struct SceneRendererData
    {
        LightingSettings Lighting;
        FogSettings Fog;
        EnvironmentSettings Environment;

        // Unified Lights (SSBO)
        static constexpr int MaxLights = 256;
        RenderLight Lights[MaxLights];
        unsigned int LightSSBO = 0;
        bool LightsDirty = true;
        int LightCount = 0;

        // Skybox
        Model SkyboxCube = {0};
        Material SkyboxMaterial = {0};
        Texture2D SkyboxCubemap = {0};
        bool SkyboxInitialized = false;

        Texture2D LightIcon = {0};
        Texture2D SpawnIcon = {0};
        Texture2D CameraIcon = {0};

        Vector3 CurrentCameraPosition = {0.0f, 0.0f, 0.0f};
        float DiagnosticMode = 0.0f;
        Timestep Time = 0.0f;
        ShaderAsset* CurrentShader = nullptr;

        // Scratch buffers reused per-frame to avoid heap allocations
        std::vector<Matrix> ScratchBoneMatrices;
        std::vector<Matrix> ScratchGlobalPose;
        std::vector<Transform> ScratchLocalPoseA;
        std::vector<Transform> ScratchLocalPoseB;
    };

private:
    struct AnimatedEntry
    {
        std::shared_ptr<class ModelAsset> asset;
        Matrix worldTransform;
        std::vector<MaterialSlot> materials;
        std::shared_ptr<class ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        AnimationComponent animation;
    };

    struct InstanceKey
    {
        size_t Hash = 0;
        InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats);
        bool operator<(const InstanceKey& o) const
        {
            return Hash < o.Hash;
        }
        bool operator==(const InstanceKey& o) const
        {
            return Hash == o.Hash;
        }
    };

    struct InstanceKeyHash
    {
        size_t operator()(const InstanceKey& k) const
        {
            return k.Hash;
        }
    };

    struct InstanceGroup
    {
        std::shared_ptr<class ModelAsset> asset;
        std::vector<Matrix> transforms;
        std::vector<MaterialSlot> materials;
    };

private:
    void PrepareLights(entt::registry& registry, const Frustum& frustum);
    void CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                            std::vector<AnimatedEntry>& animatedEntries,
                            std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);
    void DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries);
    void DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);

    void DrawColliderDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawCollisionModelBoxDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawSpawnDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);

    void ApplyFogUniforms(ShaderAsset* shader);

    // DrawModel decomposition helpers
    std::vector<Matrix> ComputeBoneMatrices(const std::shared_ptr<ModelAsset>& modelAsset, int animationIndex,
                                            float frameIndex, int targetAnimationIndex = -1,
                                            float targetFrameIndex = 0.0f, float blendWeight = 0.0f);
    Material ResolveMaterialForMesh(int meshIndex, const Model& model,
                                    const std::vector<MaterialSlot>& materialSlotOverrides);
    void BindShaderUniforms(ShaderAsset* shader, const std::vector<Matrix>& boneMatrices,
                            const std::vector<ShaderUniform>& shaderUniformOverrides);
    void BindMaterialUniforms(ShaderAsset* shader, const Material& material, int meshIndex, const Model& model,
                              const std::vector<MaterialSlot>& materialSlotOverrides);

    void InitializeSkybox();
    void DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera);

    static BoundingBox CalculateColliderWorldAABB(const ColliderComponent& collider, const Matrix& worldTransform);

    static Matrix GetWorldTransform(entt::registry& registry, entt::entity entity);
    static Vector3 GetWorldPosition(entt::registry& registry, entt::entity entity);
    static void RenderBVHNode(const class BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color,
                              int depth = 0);

private:
    static SceneRenderer* s_Instance;
    std::unique_ptr<SceneRendererData> m_Data;
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
