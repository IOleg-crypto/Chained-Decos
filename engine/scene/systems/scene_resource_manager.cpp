#include "scene_resource_manager.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/physics/iphysics_world.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/primitive_runtime.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_context.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <entt/entt.hpp>
#include <future>
#include <glm/gtx/norm.hpp>

namespace Chained::SceneResources
{

void RegisterObservers(entt::registry& reg)
{
    reg.on_construct<SpriteComponent>().connect<&ResolveSprite>();
    reg.on_update<SpriteComponent>().connect<&ResolveSprite>();

    reg.on_construct<ShaderComponent>().connect<&ResolveShader>();
    reg.on_update<ShaderComponent>().connect<&ResolveShader>();

    // Primitives only get flagged dirty here (safe off the main thread). The actual mesh /
    // VAO build happens in Update() on the main thread. on_update fires when the inspector
    // patches the component, giving live regeneration on parameter changes.
    reg.on_construct<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
    reg.on_update<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
}

// NOTE: ModelComponent is intentionally NOT connected to on_construct/on_update.
// Scene loading happens on a background thread — calling AssimpImporter from there
// is unsafe (OpenGL VAO creation requires the main thread context).
// Model resolution is handled by the Update() loop which runs on the main thread.

void Update(entt::registry& reg, Timestep ts)
{
    CH_PROFILE_FUNCTION();

    // AssetManager handles finalization internally now
    CH_PROFILE_FUNCTION();

    reg.view<RigidBodyComponent, TransformComponent>().each([&](auto entity, auto& rb, auto& transform) {
        if (rb.Handle == kInvalidPhysicsBody && reg.ctx().contains<IPhysicsWorld*>())
        {
            OnRigidBodyConstruct(reg, entity);
        }
    });

    // Asset resolution: re-attempt resolution for assets that weren't ready earlier
    reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
        if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
        {
            ResolveSprite(reg, entity);
        }
    });

    reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
        if (!shader.ShaderPath.empty() && shader.ShaderHandle == 0)
        {
            ResolveShader(reg, entity);
        }
    });

    reg.view<ModelComponent>().each([&](auto entity, auto& model) {
        if (!model.ModelPath.empty() && model.ModelHandle == 0)
        {
            ResolveModel(reg, entity);
        }
    });

    // Procedural primitives: (re)build the backing ModelAsset on the main thread when it is
    // missing or its parameters changed (Dirty). Must run before rendering.
    reg.view<PrimitiveComponent>().each([&](auto entity, auto& prim) {
        auto& rt = reg.get_or_emplace<PrimitiveRuntimeState>(entity);
        if (prim.Type != PrimitiveType::None && (!rt.Asset || rt.Dirty))
        {
            ResolvePrimitive(reg, entity);
        }
        else if (rt.TexturesPending && rt.Asset)
        {
            // Textures were still loading last frame — try to apply them now without rebuilding.
            ApplyPrimitiveTextures(reg, entity);
        }
    });

    // Animation updates
    auto animView = reg.view<AnimationComponent, ModelComponent>();
    for (auto entity : animView)
    {
        auto& animation = animView.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
        {
            continue;
        }

        auto& model = animView.get<ModelComponent>(entity);

        auto handle = ServiceLocator::Get<AssetManager>()->LoadAsset(model.ModelPath, ModelAsset::GetStaticType());
        auto modelAsset = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
        {
            continue;
        }

        int animCount = modelAsset->GetAnimationCount();
        if (animation.CurrentAnimationIndex >= animCount)
        {
            animation.CurrentAnimationIndex = 0;
            animation.CurrentFrame = 0;
        }
        if (animation.TargetAnimationIndex >= animCount)
        {
            animation.TargetAnimationIndex = -1;
        }

        float dt = ts.GetSeconds();
        animation.FrameTimeCounter += dt;

        float targetFPS = 30.0f;
        const auto& rawAnims = modelAsset->GetAnimations();
        if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
        {
            targetFPS = rawAnims[animation.CurrentAnimationIndex].frameRate;
        }
        float frameTime = 1.0f / targetFPS;

        while (animation.FrameTimeCounter >= frameTime)
        {
            animation.FrameTimeCounter -= frameTime;
            animation.CurrentFrame++;

            if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
            {
                int totalFrames = rawAnims[animation.CurrentAnimationIndex].frameCount;
                if (animation.CurrentFrame >= totalFrames)
                {
                    if (animation.IsLooping)
                    {
                        animation.CurrentFrame = 0;
                    }
                    else
                    {
                        animation.CurrentFrame = totalFrames - 1;
                        animation.IsPlaying = false;
                    }
                }
            }
        }

        // Handle Blending
        if (animation.Blending)
        {
            animation.BlendTimer += dt;
            if (animation.BlendTimer >= animation.BlendDuration)
            {
                animation.CurrentAnimationIndex = animation.TargetAnimationIndex;
                animation.CurrentFrame = animation.TargetFrame;
                animation.Blending = false;
                animation.TargetAnimationIndex = -1;
            }
            else
            {
                animation.TargetFrame++;
                if (animation.TargetAnimationIndex >= 0 &&
                    animation.TargetAnimationIndex < modelAsset->GetAnimationCount())
                {
                    int targetTotalFrames = modelAsset->GetAnimations()[animation.TargetAnimationIndex].frameCount;
                    if (animation.TargetFrame >= targetTotalFrames)
                    {
                        animation.TargetFrame = 0;
                    }
                }
            }
        }
    }

    // Audio updates
    // 1. Sync Listener with Primary Camera
    auto cameraView = reg.view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto& camera = cameraView.get<CameraComponent>(entity);
        if (camera.Primary)
        {
            auto& transform = cameraView.get<TransformComponent>(entity);
            glm::vec3 pos = glm::vec3(transform.WorldTransform[3]);
            glm::mat3 rot = glm::mat3(transform.WorldTransform);
            glm::vec3 forward = rot * glm::vec3(0, 0, -1);
            glm::vec3 up = rot * glm::vec3(0, 1, 0);

            if (auto* audioSvc = ServiceLocator::TryGet<Audio>())
                audioSvc->SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = reg.view<AudioComponent, TransformComponent>();
    auto* audioSvc = ServiceLocator::TryGet<Audio>();
    if (!audioSvc) return;
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);

        // Ensure the audio is loaded
        if (!audio.SoundPath.empty())
        {
            if (audio.SoundHandle == 0 || !audioSvc->IsSoundLoaded(audio.SoundHandle))
            {
                CH_CORE_INFO("AudioComponent: Loading sound: {}", audio.SoundPath);
                audio.SoundHandle = audioSvc->LoadSound(audio.SoundPath);
            }
        }

        // Autoplay if requested
        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            audioSvc->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop,
                                               audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }
        else if (audio.IsPlaying && audio.Spatialized && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
            audioSvc->SetInstancePosition(audio.SoundHandle, worldPos);
        }
    }
}

void OnRuntimeStart(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_INFO("SceneResources::OnRuntimeStart - Start");

    auto& registry = scene->GetRegistry();
    if (!registry.ctx().find<IPhysicsWorld*>())
    {
        CH_CORE_INFO("SceneResources::OnRuntimeStart - Need Physics World");
        // Read from the registry-scoped SceneContext, which Scene::OnRuntimeStart caches
        // just before calling here — see scene_context.h for why (EnTT callback signatures).
        auto& physics = (*registry.ctx().get<SceneContext>().PhysicsSystem);
        CH_CORE_INFO("SceneResources::OnRuntimeStart - Obtaining world pointer");
        IPhysicsWorld* world = physics.GetWorld();
        CH_CORE_INFO("SceneResources::OnRuntimeStart - World pointer obtained: {}", (void*)world);
        registry.ctx().emplace<IPhysicsWorld*>(world);
    }

    auto view = registry.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }

    CH_CORE_INFO("SceneResources::OnRuntimeStart - Done");
}

void OnRuntimeStop(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    if (auto* audioSvc = ServiceLocator::TryGet<Audio>())
    {
        audioSvc->StopAll();
    }

    auto& reg = scene->GetRegistry();
    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}

void ResolveSprite(entt::registry& reg, entt::entity e)
{
    auto& sprite = reg.get<SpriteComponent>(e);
    if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
    {

        {
            auto handle =
                ServiceLocator::Get<AssetManager>()->LoadAsset(sprite.TexturePath, TextureAsset::GetStaticType());
            auto asset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(sprite.TexturePath);
            if (asset && asset->IsReady())
            {
                sprite.TextureHandle = asset->GetID();
            }
        }
    }
}

void ResolveShader(entt::registry& reg, entt::entity e)
{
    auto& shader = reg.get<ShaderComponent>(e);
    if (shader.ShaderPath.empty() || shader.ShaderHandle != 0)
    {
        return;
    }
    {
        auto handle = ServiceLocator::Get<AssetManager>()->LoadAsset(shader.ShaderPath, ShaderAsset::GetStaticType());
        auto asset = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(shader.ShaderPath);
        if (!asset || !asset->IsReady())
        {
            return;
        }

        shader.ShaderHandle = asset->GetID();
    }
}

void ResolveModel(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);
    ComponentUtils::ResolveModelPath(model);
}

void MarkPrimitiveDirty(entt::registry& reg, entt::entity e)
{
    reg.get_or_emplace<PrimitiveRuntimeState>(e).Dirty = true;
}

void ResolvePrimitive(entt::registry& reg, entt::entity e)
{
    auto& prim = reg.get<PrimitiveComponent>(e);
    auto& rt   = reg.get_or_emplace<PrimitiveRuntimeState>(e);

    const char* typeMarker = nullptr;
    switch (prim.Type)
    {
    case PrimitiveType::Cube:
        typeMarker = ":cube:";
        break;
    case PrimitiveType::Sphere:
        typeMarker = ":sphere:";
        break;
    case PrimitiveType::Plane:
        typeMarker = ":plane:";
        break;
    case PrimitiveType::Cylinder:
        typeMarker = ":cylinder:";
        break;
    case PrimitiveType::Cone:
        typeMarker = ":cone:";
        break;
    case PrimitiveType::Torus:
        typeMarker = ":torus:";
        break;
    case PrimitiveType::Knot:
        typeMarker = ":knot:";
        break;
    case PrimitiveType::Hemisphere:
        typeMarker = ":hemisphere:";
        break;
    case PrimitiveType::None:
    default:
        rt.Dirty = false;
        return;
    }

    ProceduralParameters params;
    params.Radius = prim.Radius;
    params.InnerRadius = prim.InnerRadius;
    params.Height = prim.Height;
    params.Slices = prim.Slices;
    params.Stacks = prim.Stacks;
    params.Dimensions = prim.Dimensions;

    PendingModelData data = GeometryGenerator::GeneratePrimitivePendingData(typeMarker, params);
    if (!data.isValid)
    {
        rt.Dirty = false;
        return;
    }

    bool hadAsset = (rt.Asset != nullptr);
    std::vector<Material> editedMaterials;
    if (hadAsset)
    {
        // A rebuild replaces the whole asset, which would otherwise reset any material the user
        // tuned in the Material Editor back to the generated default.
        editedMaterials = rt.Asset->GetMaterials();
    }

    if (!rt.Asset)
    {
        rt.Asset = std::make_shared<ModelAsset>();
    }
    rt.Asset->SetPendingData(std::move(data));
    rt.Asset->OnLoaded(); // main-thread GPU upload

    // Helper: resolve texture handle from path via AssetManager.
    // Kicks async loading if not yet requested. Returns true if still pending.
    auto resolveTexture = [](AssetManager* assets, const std::string& path,
                             std::shared_ptr<Texture>& outTex) -> bool {
        if (path.empty()) return false;
        assets->LoadAsset(path, TextureAsset::GetStaticType());
        auto texAsset = assets->Get<TextureAsset>(path);
        if (texAsset && texAsset->IsReady())
        {
            outTex = texAsset->GetTexture();
            return false;
        }
        return true; // still pending
    };

    auto applyMaterialTextures = [&](Material& mat) -> bool {
        bool anyPending = false;
        if (auto* assets = ServiceLocator::TryGet<AssetManager>())
        {
            anyPending |= resolveTexture(assets, mat.AlbedoPath,              mat.AlbedoMap);
            anyPending |= resolveTexture(assets, mat.NormalPath,               mat.NormalMap);
            anyPending |= resolveTexture(assets, mat.MetallicRoughnessPath,    mat.MetallicRoughnessMap);
            anyPending |= resolveTexture(assets, mat.EmissivePath,             mat.EmissiveMap);
        }
        return anyPending;
    };

    auto& regenerated = rt.Asset->GetMaterials();
    bool anyPending = false;

    if (!editedMaterials.empty())
    {
        // Rebuild after a geometry-param change: restore materials the user had tuned,
        // and re-resolve texture handles (they may have expired or been missing earlier).
        for (size_t i = 0; i < regenerated.size() && i < editedMaterials.size(); ++i)
        {
            anyPending |= applyMaterialTextures(editedMaterials[i]);
            regenerated[i] = editedMaterials[i];
        }
        if (!regenerated.empty())
        {
            prim.SetMaterial(regenerated[0]);
        }
    }
    else
    {
        // First build: apply material stored in the component (may have been serialized).
        if (!regenerated.empty())
        {
            Material mat = prim.GetMaterial();
            anyPending = applyMaterialTextures(mat);
            regenerated[0] = mat;
        }
    }

    if (anyPending)
    {
        // Some textures are still loading. Keep asset ready for rendering (solid color
        // fallback), but mark TexturesPending so we re-apply textures next frame.
        rt.TexturesPending = true;
        rt.Dirty = false;
        return;
    }

    rt.TexturesPending = false;
    rt.Dirty = false;
}

void ApplyPrimitiveTextures(entt::registry& reg, entt::entity e)
{
    auto* rt  = reg.try_get<PrimitiveRuntimeState>(e);
    auto* prim = reg.try_get<PrimitiveComponent>(e);
    if (!rt || !rt->Asset || !prim) return;

    auto& mats = rt->Asset->GetMaterials();
    if (mats.empty()) return;

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets) return;

    Material mat = mats[0]; // working copy
    bool anyPending = false;

    auto resolveTexture = [&](const std::string& path, std::shared_ptr<Texture>& outTex) -> bool {
        if (path.empty()) return false;
        assets->LoadAsset(path, TextureAsset::GetStaticType());
        auto texAsset = assets->Get<TextureAsset>(path);
        if (texAsset && texAsset->IsReady())
        {
            outTex = texAsset->GetTexture();
            return false;
        }
        return true;
    };

    anyPending |= resolveTexture(mat.AlbedoPath,           mat.AlbedoMap);
    anyPending |= resolveTexture(mat.NormalPath,            mat.NormalMap);
    anyPending |= resolveTexture(mat.MetallicRoughnessPath, mat.MetallicRoughnessMap);
    anyPending |= resolveTexture(mat.EmissivePath,          mat.EmissiveMap);

    mats[0] = mat;
    rt->TexturesPending = anyPending;
}

bool BuildBodyDesc(entt::registry& reg, entt::entity e, PhysicsBodyDesc& outDesc)
{
    if (!reg.all_of<TransformComponent, RigidBodyComponent>(e))
    {
        return false;
    }

    auto& transform = reg.get<TransformComponent>(e);
    auto& rb = reg.get<RigidBodyComponent>(e);

    auto* collider = reg.try_get<ColliderComponent>(e);
    if (!collider || !collider->Enabled)
    {
        return false;
    }

    if (collider->AutoCalculate)
    {
        if (auto* physics = ServiceLocator::TryGet<Physics>())
        {
            physics->ApplyAutoCalculate(e, reg, *collider, transform.Scale);
        }
    }

    PhysicsBodyDesc desc;
    desc.Position = transform.Translation;
    desc.Rotation = transform.RotationQuat;
    desc.IsKinematic = (rb.Type == RigidBodyComponent::BodyType::Kinematic);
    desc.IsStatic = (rb.Type == RigidBodyComponent::BodyType::Static);
    desc.Mass = rb.Mass;
    desc.LinearDamping = rb.LinearDamping;
    desc.AngularDamping = rb.AngularDamping;
    desc.UseGravity = rb.UseGravity;
    desc.IsFixedRotation = rb.IsFixedRotation;
    desc.InitialVelocity = rb.Velocity;
    desc.UserData = (uint64_t)e;

    desc.Shape = collider->Type;
    desc.Friction = collider->Friction;
    desc.Restitution = collider->Restitution;
    desc.Offset = collider->Offset;
    desc.UseFastBuildQuality = collider->UseFastBuildQuality;

    switch (collider->Type)
    {
    case ColliderType::Box:
        desc.Dimensions = (collider->Size * transform.Scale) * 0.5f;
        break;

    case ColliderType::Sphere:
        desc.Dimensions.x = collider->Radius * std::max({transform.Scale.x, transform.Scale.y, transform.Scale.z});
        break;

    case ColliderType::Capsule:
        desc.Dimensions.x = collider->Radius * std::max(transform.Scale.x, transform.Scale.z);
        desc.Dimensions.y = (collider->Height * transform.Scale.y) * 0.5f;
        break;

    case ColliderType::Mesh: {
        std::string modelPath = collider->ModelPath;
        if (modelPath.empty())
        {
            if (auto* modelComp = reg.try_get<ModelComponent>(e))
            {
                modelPath = modelComp->ModelPath;
            }
        }

        if (modelPath.empty())
        {
            return false;
        }

        if (auto* worldPtr = reg.ctx().find<IPhysicsWorld*>())
        {
            if ((*worldPtr) && (*worldPtr)->HasCachedMeshShape(modelPath))
            {
                desc.CacheKey = modelPath;
                desc.MeshScale = transform.Scale;
                break;
            }
        }

        auto modelAsset = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(modelPath);
        if (!modelAsset || !modelAsset->IsReady())
        {
            return false;
        }

        const auto& rawMeshes = modelAsset->GetRawMeshes();
        const auto& instances = modelAsset->GetInstances();

        desc.MeshScale = transform.Scale;
        desc.CacheKey = modelPath;

        for (const auto& inst : instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= (int)rawMeshes.size())
            {
                continue;
            }

            const RawMesh& raw = rawMeshes[inst.meshIndex];
            if (raw.indices.size() < 3)
            {
                continue;
            }

            const glm::mat4& meshToLocal = inst.localTransform;

            for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
            {
                uint32_t i0 = raw.indices[i];
                uint32_t i1 = raw.indices[i + 1];
                uint32_t i2 = raw.indices[i + 2];

                size_t v0Idx = (size_t)i0 * 3;
                size_t v1Idx = (size_t)i1 * 3;
                size_t v2Idx = (size_t)i2 * 3;

                if (v0Idx + 2 >= raw.vertices.size() || v1Idx + 2 >= raw.vertices.size() ||
                    v2Idx + 2 >= raw.vertices.size())
                {
                    continue;
                }

                glm::vec3 v0 = {raw.vertices[v0Idx], raw.vertices[v0Idx + 1], raw.vertices[v0Idx + 2]};
                glm::vec3 v1 = {raw.vertices[v1Idx], raw.vertices[v1Idx + 1], raw.vertices[v1Idx + 2]};
                glm::vec3 v2 = {raw.vertices[v2Idx], raw.vertices[v2Idx + 1], raw.vertices[v2Idx + 2]};

                PhysicsTriangle tri;
                tri.V0 = glm::vec3(meshToLocal * glm::vec4(v0, 1.0f));
                tri.V1 = glm::vec3(meshToLocal * glm::vec4(v1, 1.0f));
                tri.V2 = glm::vec3(meshToLocal * glm::vec4(v2, 1.0f));

                desc.Triangles.push_back(tri);
            }
        }
        break;
    }
    }

    outDesc = std::move(desc);
    return true;
}

void OnRigidBodyConstruct(entt::registry& reg, entt::entity e)
{
    if (!reg.ctx().contains<IPhysicsWorld*>())
    {
        return;
    }

    auto world = reg.ctx().get<SceneContext>().PhysicsSystem->GetWorld();
    if (!world)
    {
        return;
    }

    auto& rb = reg.get<RigidBodyComponent>(e);

    if (rb.Handle != kInvalidPhysicsBody)
    {
        return;
    }

    auto* collider = reg.try_get<ColliderComponent>(e);
    if (!collider || !collider->Enabled)
    {
        return;
    }

    PhysicsBodyDesc desc;
    if (!BuildBodyDesc(reg, e, desc))
    {
        return;
    }

    rb.Handle = world->CreateBody(desc);
}

void BatchInitializeBodies(entt::registry& reg, IPhysicsWorld* world)
{
    struct PendingBody
    {
        entt::entity entity;
        PhysicsBodyDesc desc;
        int sortOrder;
    };

    std::vector<PendingBody> pending;

    // Phase 1: Collect candidates + apply AutoCalculate (writes to collider, must be sequential)
    auto view = reg.view<RigidBodyComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& rb = view.get<RigidBodyComponent>(entity);
        if (rb.Handle != kInvalidPhysicsBody)
        {
            continue;
        }

        auto* collider = reg.try_get<ColliderComponent>(entity);
        if (!collider || !collider->Enabled)
        {
            continue;
        }

        if (collider->AutoCalculate)
        {
            auto& transform = view.get<TransformComponent>(entity);
            if (auto* physics = ServiceLocator::TryGet<Physics>())
            {
                physics->ApplyAutoCalculate(entity, reg, *collider, transform.Scale);
            }
        }

        pending.push_back({entity, {}, 0});
    }

    if (pending.empty())
    {
        return;
    }

    // Phase 2: Build BodyDescs in parallel (read-only phase — safe for concurrent access)
    // Triangle extraction from model assets is the bottleneck for Mesh colliders.
    size_t parallelThreshold = 4;
    if (pending.size() >= parallelThreshold)
    {
        std::atomic<bool> buildFailed{false};
        std::vector<std::future<void>> futures;
        futures.reserve(pending.size());

        for (size_t i = 0; i < pending.size(); ++i)
        {
            futures.push_back(std::async(std::launch::async, [&reg, &pending, i, &buildFailed]() {
                if (!BuildBodyDesc(reg, pending[i].entity, pending[i].desc))
                {
                    buildFailed.store(true, std::memory_order_relaxed);
                }
            }));
        }
        for (auto& f : futures)
            f.get();

        // Remove failed entries
        if (buildFailed.load())
        {
            pending.erase(
                std::remove_if(pending.begin(), pending.end(),
                               [](const PendingBody& pb) { return pb.desc.Triangles.empty() && pb.desc.Shape == ColliderType::Mesh; }),
                pending.end());
        }
    }
    else
    {
        // Few bodies: sequential is faster
        auto it = pending.begin();
        while (it != pending.end())
        {
            if (!BuildBodyDesc(reg, it->entity, it->desc))
            {
                it = pending.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Set sort order and move descs
    for (auto& pb : pending)
    {
        pb.sortOrder = pb.desc.IsStatic ? 0 : (pb.desc.IsKinematic ? 1 : 2);
    }

    if (pending.empty())
    {
        return;
    }

    std::sort(pending.begin(), pending.end(),
              [](const PendingBody& a, const PendingBody& b) { return a.sortOrder < b.sortOrder; });

    std::vector<PhysicsBodyDesc> descs;
    descs.reserve(pending.size());
    for (auto& pb : pending)
    {
        descs.push_back(std::move(pb.desc));
    }

    auto handles = world->CreateBodies(descs);

    for (size_t i = 0; i < pending.size(); ++i)
    {
        auto& rb = reg.get<RigidBodyComponent>(pending[i].entity);
        rb.Handle = handles[i];
    }

    CH_CORE_INFO("Physics: Batch-created {} bodies (static-first).", pending.size());
}

} // namespace Chained::SceneResources