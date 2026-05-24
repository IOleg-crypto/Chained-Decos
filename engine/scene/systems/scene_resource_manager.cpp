#include "scene_resource_manager.h"
#include "engine/scene/scene.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/components/component_utils.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include <glm/gtx/norm.hpp>
#include <cmath>

namespace CHEngine
{

void SceneResourceManager::RegisterObservers(entt::registry& reg)
{
    reg.on_construct<SpriteComponent>().connect<&SceneResourceManager::OnSpriteChanged>(this);
    reg.on_update<SpriteComponent>().connect<&SceneResourceManager::OnSpriteChanged>(this);

    reg.on_construct<ShaderComponent>().connect<&SceneResourceManager::OnShaderChanged>(this);
    reg.on_update<ShaderComponent>().connect<&SceneResourceManager::OnShaderChanged>(this);

    reg.on_construct<ModelComponent>().connect<&SceneResourceManager::OnModelChanged>(this);
    reg.on_update<ModelComponent>().connect<&SceneResourceManager::OnModelChanged>(this);

    // Expose pointer to this manager via registry context for other systems/tools
    if (!reg.ctx().contains<SceneResourceManager*>())
        reg.ctx().emplace<SceneResourceManager*>(this);
}

void SceneResourceManager::OnUpdate(Scene* scene, Timestep ts)
{
    CH_PROFILE_FUNCTION();
    auto& reg = scene->GetRegistry();

    // Asset resolution: re-attempt resolution for assets that weren't ready earlier
    reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
        if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
            OnSpriteChanged(reg, entity);
    });

    reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
        if (!shader.ShaderPath.empty() && shader.ShaderHandle == 0)
            OnShaderChanged(reg, entity);
    });

    reg.view<ModelComponent>().each([&](auto entity, auto& model) {
        if (!model.ModelPath.empty() && (model.ModelHandle == 0 || !model.MaterialsInitialized))
            OnModelChanged(reg, entity);
    });

    // Animation updates
    auto animView = reg.view<AnimationComponent, ModelComponent>();
    for (auto entity : animView)
    {
        auto& animation = animView.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
            continue;

        auto& model = animView.get<ModelComponent>(entity);
        AssetManager* assetManager = nullptr;
        if (reg.ctx().contains<AssetManager*>())
            assetManager = reg.ctx().get<AssetManager*>();
        if (!assetManager && ServiceLocator::Has<AssetManager>())
            assetManager = &ServiceLocator::Get<AssetManager>();

        if (!assetManager)
            continue;

        auto handle = assetManager->ResolveToHandle(model.ModelPath, ModelAsset::GetStaticType());
        auto modelAsset = assetManager->Get<ModelAsset>(handle);
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

            Audio* audioSvc = nullptr;
            if (reg.ctx().contains<Audio*>())
                audioSvc = reg.ctx().get<Audio*>();
            if (!audioSvc && ServiceLocator::Has<Audio>())
                audioSvc = &ServiceLocator::Get<Audio>();
            if (audioSvc)
                audioSvc->SetListenerPosition(pos, forward, up);
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
            Audio* audioSvc = nullptr;
            if (reg.ctx().contains<Audio*>())
                audioSvc = reg.ctx().get<Audio*>();
            if (!audioSvc && ServiceLocator::Has<Audio>())
                audioSvc = &ServiceLocator::Get<Audio>();
            if (audio.SoundHandle == 0 && audioSvc)
            {
                if (!audioSvc->IsSoundLoaded(audio.SoundHandle))
                    audio.SoundHandle = audioSvc->LoadSound(audio.SoundPath);
            }
        }

        // Autoplay if requested
        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            Audio* audioSvc = nullptr;
            if (reg.ctx().contains<Audio*>())
                audioSvc = reg.ctx().get<Audio*>();
            if (!audioSvc && ServiceLocator::Has<Audio>())
                audioSvc = &ServiceLocator::Get<Audio>();
            if (audioSvc)
                audioSvc->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }
        else if (audio.IsPlaying && audio.Spatialized && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
            Audio* audioSvc = nullptr;
            if (reg.ctx().contains<Audio*>())
                audioSvc = reg.ctx().get<Audio*>();
            if (!audioSvc && ServiceLocator::Has<Audio>())
                audioSvc = &ServiceLocator::Get<Audio>();
            if (audioSvc)
                audioSvc->SetInstancePosition(audio.SoundHandle, worldPos);
        }
    }
}

void SceneResourceManager::OnRuntimeStart(Scene* scene)
{
    CH_PROFILE_FUNCTION();
}

void SceneResourceManager::OnRuntimeStop(Scene* scene)
{
    CH_PROFILE_FUNCTION();
    auto& reg = scene->GetRegistry();

    Audio* audioSvc = nullptr;
    if (reg.ctx().contains<Audio*>())
        audioSvc = reg.ctx().get<Audio*>();
    if (!audioSvc && ServiceLocator::Has<Audio>())
        audioSvc = &ServiceLocator::Get<Audio>();
    if (audioSvc)
        audioSvc->StopAll();

    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}

void SceneResourceManager::OnUpdateEditor(Scene* scene, Timestep ts)
{
    CH_PROFILE_FUNCTION();
    // Reuse the same update path for assets + animation in editor
    OnUpdate(scene, ts);
}

void SceneResourceManager::OnSpriteChanged(entt::registry& reg, entt::entity e)
{
    auto& sprite = reg.get<SpriteComponent>(e);
    if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
    {
        AssetManager* assetManager = nullptr;
        if (reg.ctx().contains<AssetManager*>())
            assetManager = reg.ctx().get<AssetManager*>();
        if (!assetManager && ServiceLocator::Has<AssetManager>())
            assetManager = &ServiceLocator::Get<AssetManager>();

        if (assetManager)
        {
            auto handle = assetManager->ResolveToHandle(sprite.TexturePath, TextureAsset::GetStaticType());
            auto asset = assetManager->Get<TextureAsset>(handle);
            if (asset && asset->IsReady())
            {
                sprite.TextureHandle = asset->GetID();
            }
        }
    }
}

void SceneResourceManager::OnShaderChanged(entt::registry& reg, entt::entity e)
{
    auto& shader = reg.get<ShaderComponent>(e);
    if (shader.ShaderPath.empty() || shader.ShaderHandle != 0)
        return;

    AssetManager* assetManager = nullptr;
    if (reg.ctx().contains<AssetManager*>())
        assetManager = reg.ctx().get<AssetManager*>();
    if (!assetManager && ServiceLocator::Has<AssetManager>())
        assetManager = &ServiceLocator::Get<AssetManager>();

    if (!assetManager)
        return;

    auto handle = assetManager->ResolveToHandle(shader.ShaderPath, ShaderAsset::GetStaticType());
    auto asset = assetManager->Get<ShaderAsset>(handle);
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

void SceneResourceManager::OnModelChanged(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);
    AssetManager* assetManager = nullptr;
    if (reg.ctx().contains<AssetManager*>())
        assetManager = reg.ctx().get<AssetManager*>();
    if (!assetManager && ServiceLocator::Has<AssetManager>())
        assetManager = &ServiceLocator::Get<AssetManager>();

    ComponentUtils::ResolveModelPath(model);
    if (assetManager)
    {
        ComponentUtils::SyncMaterials(model, model.ModelHandle, assetManager);
    }
    else
    {
        ComponentUtils::SyncMaterials(model, model.ModelHandle, nullptr);
    }
}

} // namespace CHEngine

