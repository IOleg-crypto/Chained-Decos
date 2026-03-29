#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include <cmath>
#include <glm/gtx/norm.hpp>

namespace CHEngine
{
// Scene implementation
Scene::Scene()
{
    // Create registry and manager handle
    auto registry = std::make_shared<entt::registry>();
    m_Manager = {entt::null, registry};

    auto& reg = GetRegistry();
    reg.ctx().emplace<Scene*>(this);
    reg.ctx().emplace<EntityUUIDMap>();
    reg.ctx().emplace<std::shared_ptr<entt::registry>>(registry);

    // UUID Mapping
    reg.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    reg.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    // Hierarchy Mapping
    reg.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyDestroy>(this);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();
}

Scene::~Scene()
{
    // Clean up active signals
    GetRegistry().clear();
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_INFO("Scene::Copy - Starting copy of scene '{}'", other->m_Settings.Name);

    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // 1. Copy Scene Settings
    newScene->m_Settings = other->m_Settings;

    // 2. Copy Entities (Direct Memory Copy)
    auto& srcRegistry = other->GetRegistry();
    auto& dstRegistry = newScene->GetRegistry();

    // Copy all entities using ComponentSerializer
    int entityCount = 0;
    srcRegistry.view<IDComponent>().each([&](auto entityHandle, auto& id) {
        entityCount++;
        Entity srcEntity = {entityHandle, other->m_Manager.GetRegistryPtr()};
        Entity dstEntity = newScene->CreateEntityWithUUID(id.ID);

        ComponentSerializer::Get().CopyAll(srcEntity, dstEntity);
    });

    CH_CORE_INFO("Scene::Copy - Successfully copied {} entities", entityCount);
    return newScene;
}

void Scene::OnHierarchyDestroy(entt::registry& reg, entt::entity entity)
{
    auto& hc = reg.get<HierarchyComponent>(entity);

    // 1. Detach from parent
    if (hc.Parent != entt::null && reg.valid(hc.Parent) && reg.all_of<HierarchyComponent>(hc.Parent))
    {
        auto& phc = reg.get<HierarchyComponent>(hc.Parent);
        auto it = std::find(phc.Children.begin(), phc.Children.end(), entity);
        if (it != phc.Children.end())
        {
            phc.Children.erase(it);
        }
    }

    // Children are handled by recursive DestroyEntity call
}

void Scene::OnRuntimeStart()
{
    m_IsSimulationRunning = true;
}

void Scene::OnRuntimeStop()
{
    m_IsSimulationRunning = false;
}

void Scene::OnUpdateRuntime(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateHierarchy();
    UpdateAnimations(timestep);
    UpdatePhysics(timestep);
    UpdateAudio(timestep);
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateHierarchy();
    UpdateAnimations(timestep);
    UpdatePhysics(timestep);
    UpdateAudio(timestep);
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
    auto& reg = GetRegistry();
    auto view = reg.view<CameraComponent>();
    for (auto entity : view)
    {
        auto& cameraComponent = view.get<CameraComponent>(entity);
        if (!cameraComponent.FixedAspectRatio)
        {
            cameraComponent.Camera.SetViewportSize(width, height);
        }
    }
}

void Scene::OnEvent(Event& event)
{
}

std::optional<Camera3D> Scene::GetActiveCamera()
{
    auto& reg = GetRegistry();
    auto view = reg.view<CameraComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto [camera, transform] = view.get<CameraComponent, TransformComponent>(entity);
        if (camera.Primary)
        {
            Camera3D activeCamera;

            // Use WorldTransform instead of local translation/rotation for parented cameras
            const glm::mat4& worldTransform = transform.WorldTransform;

            // Extract position from world transform
            activeCamera.Position = glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            // Calculate world forward and up vectors
            glm::vec3 worldForward = glm::vec3(worldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f));
            glm::vec3 worldUp = glm::vec3(worldTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

            glm::vec3 forward = glm::normalize(worldForward - activeCamera.Position);

            // Provide a safe fallback if forward is somehow zero
            if (glm::length2(forward) < 0.0001f)
            {
                forward = glm::vec3(0.0f, 0.0f, -1.0f);
            }

            activeCamera.Target = activeCamera.Position + forward;
            activeCamera.Up = glm::normalize(worldUp - activeCamera.Position);

            // SceneCamera stores FOV in radians, raylib expects degrees for Camera3D
            activeCamera.Fovy = glm::degrees(camera.Camera.GetPerspectiveVerticalFOV());
            activeCamera.Projection = (int)camera.Camera.GetProjectionType();

            return activeCamera;
        }
    }
    return std::nullopt;
}

Entity Scene::GetPrimaryCameraEntity()
{
    auto& reg = GetRegistry();
    auto view = reg.view<CameraComponent>();
    for (auto entity : view)
    {
        auto& camera = view.get<CameraComponent>(entity);
        if (camera.Primary)
        {
            return {entity, m_Manager.GetRegistryPtr()};
        }
    }
    return {};
}

void Scene::UpdatePhysics(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();
    Physics::Update(this, deltaTime, m_IsSimulationRunning);
}

void Scene::UpdateAnimations(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();
    auto& reg = GetRegistry();
    auto view = reg.view<AnimationComponent, ModelComponent>();

    for (auto entity : view)
    {
        auto& animation = view.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
        {
            continue;
        }

        auto& model = view.get<ModelComponent>(entity);
        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
        {
            continue;
        }

        // Progress timers
        float dt = deltaTime.GetSeconds();
        animation.FrameTimeCounter += dt;

        // Get animation frameRate from asset (asset-driven, defaults to 30fps if not available)
        float targetFPS = 30.0f;
        const auto& rawAnims = modelAsset->GetAnimations();
        if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
        {
            targetFPS = rawAnims[animation.CurrentAnimationIndex].frameRate;
        }
        float frameTime = 1.0f / targetFPS;

        // Use a while loop to correctly consume elapsed time without dropping fractional MS,
        // which prevents animation sequence shaking/jittering.
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
                // Blend complete
                animation.CurrentAnimationIndex = animation.TargetAnimationIndex;
                animation.CurrentFrame = animation.TargetFrame;
                animation.Blending = false;
                animation.TargetAnimationIndex = -1;
            }
            else
            {
                // Progress target frame too
                const float FRAME_EPSILON = 0.001f;
                if (std::abs(animation.FrameTimeCounter - 0.0f) < FRAME_EPSILON) // Just advanced a frame
                {
                    animation.TargetFrame++;
                    if (animation.TargetAnimationIndex >= 0 &&
                        animation.TargetAnimationIndex < modelAsset->GetAnimationCount())
                    {
                        int targetTotalFrames =
                            modelAsset->GetAnimations()[animation.TargetAnimationIndex].frameCount;
                        if (animation.TargetFrame >= targetTotalFrames)
                        {
                            animation.TargetFrame = 0;
                        }
                    }
                }
            }
        }
    }
}

void Scene::UpdateHierarchy()
{
    CH_PROFILE_FUNCTION();
    auto& reg = GetRegistry();
    auto view = reg.view<TransformComponent>();

    struct UpdateTask
    {
        entt::entity Entity;
        glm::mat4 ParentTransform;
    };

    std::vector<UpdateTask> stack;
    stack.reserve(reg.storage<entt::entity>().size());

    // 1. Find all root entities and push to stack
    for (auto entity : view)
    {
        bool isRoot = true;
        if (reg.all_of<HierarchyComponent>(entity))
        {
            auto& hc = reg.get<HierarchyComponent>(entity);
            if (hc.Parent != entt::null && reg.valid(hc.Parent) && reg.all_of<TransformComponent>(hc.Parent))
            {
                isRoot = false;
            }
        }

        if (isRoot)
        {
            stack.push_back({entity, glm::mat4(1.0f)});
        }
    }

    // 2. Iterative DFS update
    while (!stack.empty())
    {
        UpdateTask task = stack.back();
        stack.pop_back();

        auto& tc = view.get<TransformComponent>(task.Entity);
        tc.WorldTransform = task.ParentTransform * tc.GetTransform();
        tc.IsDirty = false;

        if (reg.all_of<HierarchyComponent>(task.Entity))
        {
            auto& hc = reg.get<HierarchyComponent>(task.Entity);
            for (auto child : hc.Children)
            {
                if (reg.valid(child) && reg.all_of<TransformComponent>(child))
                {
                    stack.push_back({child, tc.WorldTransform});
                }
            }
        }
    }
}

void Scene::UpdateAudio(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();

    // 1. Sync Listener with Primary Camera
    auto cameraView = GetRegistry().view<CameraComponent, TransformComponent>();
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

            Audio::Get().SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = GetRegistry().view<AudioComponent, TransformComponent>();
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);
        if (audio.PlayOnStart && !audio.IsPlaying && audio.Asset && audio.Asset->GetState() == AssetState::Ready)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            Audio::Get().Play(audio.Asset->GetBuffer(), audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized,
                              worldPos);
            audio.IsPlaying = true;
        }
    }
}

void Scene::OnIDConstruct(entt::registry& reg, entt::entity entity)
{
    auto& id = reg.get<IDComponent>(entity);
    auto& mapStruct = reg.ctx().get<EntityUUIDMap>();
    mapStruct.Map[id.ID] = entity;
}

void Scene::OnIDDestroy(entt::registry& reg, entt::entity entity)
{
    auto& id = reg.get<IDComponent>(entity);
    auto& mapStruct = reg.ctx().get<EntityUUIDMap>();
    mapStruct.Map.erase(id.ID);
}

} // namespace CHEngine
