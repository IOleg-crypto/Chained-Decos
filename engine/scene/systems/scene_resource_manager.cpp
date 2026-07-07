#include "scene_resource_manager.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/iphysics_world.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/scene.h"
#include <cmath>
#include <entt/entt.hpp>
#include <glm/gtx/norm.hpp>

namespace Chained
{

SceneResourceManager::SceneResourceManager()
{
}

void SceneResourceManager::RegisterObservers(entt::registry& reg)
{
    reg.on_construct<SpriteComponent>().connect<&SceneResourceManager::ResolveSprite>(*this);
    reg.on_update<SpriteComponent>().connect<&SceneResourceManager::ResolveSprite>(*this);

    reg.on_construct<ShaderComponent>().connect<&SceneResourceManager::ResolveShader>(*this);
    reg.on_update<ShaderComponent>().connect<&SceneResourceManager::ResolveShader>(*this);
}

// NOTE: ModelComponent is intentionally NOT connected to on_construct/on_update.
// Scene loading happens on a background thread — calling AssimpImporter from there
// is unsafe (OpenGL VAO creation requires the main thread context).
// Model resolution is handled by the Update() loop which runs on the main thread.
} // namespace Chained

void SceneResourceManager::Update(entt::registry& reg, Timestep ts)
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
                const float FRAME_EPSILON = 0.001f;
                if (std::abs(animation.FrameTimeCounter - 0.0f) < FRAME_EPSILON)
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

            ServiceLocator::Get<Audio>()->SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = reg.view<AudioComponent, TransformComponent>();
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);

        // Ensure the audio is loaded
        if (!audio.SoundPath.empty())
        {
            if (audio.SoundHandle == 0)
            {
                if (!ServiceLocator::Get<Audio>()->IsSoundLoaded(audio.SoundHandle))
                {
                    audio.SoundHandle = ServiceLocator::Get<Audio>()->LoadSound(audio.SoundPath);
                }
            }
        }

        // Autoplay if requested
        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            ServiceLocator::Get<Audio>()->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop,
                                               audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }
        else if (audio.IsPlaying && audio.Spatialized && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
            ServiceLocator::Get<Audio>()->SetInstancePosition(audio.SoundHandle, worldPos);
        }
    }
}

void SceneResourceManager::OnRuntimeStart(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - Start");

    auto& registry = scene->GetRegistry();
    if (!registry.ctx().find<IPhysicsWorld*>())
    {
        CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - Need Physics World");
        auto& physics = (*ServiceLocator::Get<Physics>());
        CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - Obtaining world pointer");
        IPhysicsWorld* world = physics.GetWorld();
        CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - World pointer obtained: {}", (void*)world);
        registry.ctx().emplace<IPhysicsWorld*>(world);
    }

    auto& physics = (*ServiceLocator::Get<Physics>());
    CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - Initializing bodies");
    physics.InitializeBodies(scene);
    CH_CORE_INFO("SceneResourceManager::OnRuntimeStart - Done");
}

void SceneResourceManager::OnRuntimeStop(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    auto& audioSvc = (*ServiceLocator::Get<Audio>());
    audioSvc.StopAll();

    auto& reg = scene->GetRegistry();
    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}

void SceneResourceManager::ResolveSprite(entt::registry& reg, entt::entity e)
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

void SceneResourceManager::ResolveShader(entt::registry& reg, entt::entity e)
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

void SceneResourceManager::ResolveModel(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);
    ComponentUtils::ResolveModelPath(model);
}

void SceneResourceManager::OnRigidBodyConstruct(entt::registry& reg, entt::entity e)
{
    if (!reg.ctx().contains<IPhysicsWorld*>())
    {
        return;
    }

    auto world = ServiceLocator::Get<Physics>()->GetWorld();
    if (!world)
    {
        return;
    }

    auto& transform = reg.get<TransformComponent>(e);
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
        desc.IsStatic = true;

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
            return;
        }

        auto modelAsset = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(modelPath);
        if (!modelAsset || !modelAsset->IsReady())
        {
            return;
        }

        const auto& rawMeshes = modelAsset->GetRawMeshes();
        const auto& instances = modelAsset->GetInstances();

        
        glm::vec3 colliderOffset = collider->Offset;

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

            
            glm::mat4 meshToLocal = glm::scale(glm::mat4(1.0f), transform.Scale) * inst.localTransform;

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

                
                tri.V0 += colliderOffset;
                tri.V1 += colliderOffset;
                tri.V2 += colliderOffset;

                desc.Triangles.push_back(tri);
            }
        }
        break;
    }
    }
    rb.Handle = world->CreateBody(desc);
}
