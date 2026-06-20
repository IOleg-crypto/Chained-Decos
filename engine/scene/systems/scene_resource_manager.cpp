#include "scene_resource_manager.h"
#include "engine/core/service_locator.h"
#include <entt/entt.hpp>
#include "engine/scene/scene.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/components/component_utils.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/physics/iphysics_world.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include <glm/gtx/norm.hpp>
#include <cmath>

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

    reg.on_construct<ModelComponent>().connect<&SceneResourceManager::ResolveModel>(*this);
    reg.on_update<ModelComponent>().connect<&SceneResourceManager::ResolveModel>(*this);
}

void SceneResourceManager::Update(entt::registry& reg, Timestep ts)
{
    CH_PROFILE_FUNCTION();

    // AssetManager handles finalization internally now

    // Asset resolution: re-attempt resolution for assets that weren't ready earlier
    reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
        if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
            ResolveSprite(reg, entity);
    });

    reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
        if (!shader.ShaderPath.empty() && shader.ShaderHandle == 0)
            ResolveShader(reg, entity);
    });

    reg.view<ModelComponent>().each([&](auto entity, auto& model) {
        if (!model.ModelPath.empty() && model.ModelHandle == 0)
            ResolveModel(reg, entity);
    });

    // Animation updates
    auto animView = reg.view<AnimationComponent, ModelComponent>();
    for (auto entity : animView)
    {
        auto& animation = animView.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
            continue;

        auto& model = animView.get<ModelComponent>(entity);


        auto handle = ServiceLocator::Get<AssetManager>()->ImportAsset(model.ModelPath);
        auto modelAsset = ServiceLocator::Get<AssetManager>()->GetAsset<ModelAsset>(handle);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
            continue;

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
                    audio.SoundHandle = ServiceLocator::Get<Audio>()->LoadSound(audio.SoundPath);
            }
        }

        // Autoplay if requested
        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            ServiceLocator::Get<Audio>()->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
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
            auto handle = ServiceLocator::Get<AssetManager>()->ImportAsset(sprite.TexturePath);
            auto asset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(handle);
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
        return;
    {
        auto handle = ServiceLocator::Get<AssetManager>()->ImportAsset(shader.ShaderPath);
        auto asset = ServiceLocator::Get<AssetManager>()->GetAsset<ShaderAsset>(handle);
        if (!asset || !asset->IsReady())
            return;

        shader.ShaderHandle = asset->GetID();
        const auto& assetUniforms = asset->GetUniforms();
        for (const auto& u : assetUniforms)
        {
            auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(),
                                   [&](const auto& current) { return current.Name == u.Name; });
            if (it == shader.Uniforms.end())
                shader.Uniforms.push_back(u);
        }
    }
}

void SceneResourceManager::ResolveModel(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);
    ComponentUtils::ResolveModelPath(model);
}


} // namespace Chained

