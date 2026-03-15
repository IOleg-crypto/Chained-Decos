#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/scene_events.h"
#include "project.h"
#include "raylib.h"
#include "raymath.h"
#include "scene_serializer.h"
#include "scripting/scene_scripting.h"

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
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateHierarchy();
    UpdateAnimations(timestep);
    UpdatePhysics(timestep);
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
            Camera3D raylibCamera;
            raylibCamera.position = transform.Translation;
            
            // Calculate forward and up vectors from rotation
            Vector3 forward = Vector3RotateByQuaternion(Vector3{ 0, 0, -1 }, transform.RotationQuat);
            raylibCamera.target = Vector3Add(transform.Translation, forward);
            raylibCamera.up = Vector3RotateByQuaternion(Vector3{ 0, 1, 0 }, transform.RotationQuat);
            
            // SceneCamera stores FOV in radians, raylib expects degrees for Camera3D
            raylibCamera.fovy = camera.Camera.GetPerspectiveVerticalFOV() * RAD2DEG;
            raylibCamera.projection = (int)camera.Camera.GetProjectionType();
            
            return raylibCamera;
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
        auto& model = view.get<ModelComponent>(entity);

        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (modelAsset && animation.CurrentAnimationIndex < modelAsset->GetAnimationCount())
        {
            // Update animation logic here
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
        Matrix ParentTransform;
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
            if (hc.Parent != entt::null && reg.valid(hc.Parent) &&
                reg.all_of<TransformComponent>(hc.Parent))
            {
                isRoot = false;
            }
        }

        if (isRoot)
        {
            stack.push_back({entity, MatrixIdentity()});
        }
    }

    // 2. Iterative DFS update
    while (!stack.empty())
    {
        UpdateTask task = stack.back();
        stack.pop_back();

        auto& tc = view.get<TransformComponent>(task.Entity);
        tc.WorldTransform = MatrixMultiply(tc.GetTransform(), task.ParentTransform);
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
