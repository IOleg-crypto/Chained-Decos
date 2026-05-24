#include "engine/scene/scene.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/physics/default_physics_world.h"
#include "engine/physics/physics.h"
#include "engine/physics/physics_system.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/scene_system_manager.h"
#include "engine/scene/systems/scene_resource_manager.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/audio/audio.h"
#include "engine/scene/systems/scene_systems_impl.h"
#include "engine/scene/animation_systems.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include "engine/scene/ui_factory.h"
#include "scene_scripting_manager.h"
#include "scripting/scriptengine.h"
#include <entt/entt.hpp>
#include <glm/gtx/norm.hpp>

using namespace entt::literals;
#include "engine/scene/systems/scene_transition_system.h"

namespace CHEngine
{
// Scene implementation
Scene::Scene(ScriptEngine* scriptEngine)
{
    // Create registry
    m_Registry = std::make_unique<entt::registry>();
    auto& reg = *m_Registry;
    reg.ctx().emplace<Scene*>(this);
    reg.ctx().emplace<EntityUUIDMap>();
    // Inject commonly used engine services into the registry context so scene systems
    // and serializers can access them without using the global ServiceLocator.
    if (ServiceLocator::Has<AssetManager>())
        reg.ctx().emplace<AssetManager*>(&ServiceLocator::Get<AssetManager>());
    if (ServiceLocator::Has<ComponentSerializer>())
        reg.ctx().emplace<ComponentSerializer*>(&ServiceLocator::Get<ComponentSerializer>());
    if (ServiceLocator::Has<UIRenderer>())
        reg.ctx().emplace<UIRenderer*>(&ServiceLocator::Get<UIRenderer>());
    if (ServiceLocator::Has<Audio>())
        reg.ctx().emplace<Audio*>(&ServiceLocator::Get<Audio>());

    // UUID Mapping
    reg.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    reg.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    // Hierarchy Mapping
    reg.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyDestroy>(this);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();

    // Initialize managed systems
    m_ScriptingManager = std::make_unique<SceneScriptingManager>(this, scriptEngine);
    m_SystemManager = std::make_unique<SceneSystemManager>(this);

    // Register Core ECS Systems (inject PhysicsSystem dependency)
    m_SystemManager->AddSystem<SceneRuntimeUpdater>(ServiceLocator::Get<PhysicsSystem>());
    // Consolidated resource manager replaces the separate asset/animation/audio systems
    m_SystemManager->AddSystem<SceneResourceManager>();
    m_SystemManager->AddSystem<SceneTransitionSystem>();

    m_SystemManager->InitObservers();

    UIFactory::Initialize();
}

Scene::~Scene()
{
    Physics::ClearContext(this);
    // Clean up active signals
    GetRegistry().clear();
}

std::shared_ptr<Scene> Scene::CreateDefault()
{
    // Use the active application's ScriptEngine or nullptr if not available
    auto scene = std::make_shared<Scene>(nullptr);

    // Ensure every scene starts with a Main Camera
    Entity camera = scene->CreateEntity("Main Camera");
    auto& cameraEntity = camera.AddComponent<CameraComponent>();
    cameraEntity.Primary = true;
    camera.GetComponent<TransformComponent>().Translation = {0, 5, 10};

    return scene;
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_INFO("Scene::Copy - Starting copy of scene '{}'", other->m_Settings.Name);

    std::shared_ptr<Scene> newScene = std::make_shared<Scene>(other->GetScriptEngine());

    // 1. Copy Scene Settings
    newScene->m_Settings = other->m_Settings;

    // 2. Copy Entities (remap handled via ID)
    auto& srcRegistry = other->GetRegistry();
    auto& dstRegistry = newScene->GetRegistry();

    std::unordered_map<entt::entity, entt::entity> entityMap;

    // First pass: Create all entities and copy basic components
    {
        CH_PROFILE_SCOPE("Scene::Copy::CopyEntities_Pass1");
        srcRegistry.view<IDComponent>().each([&](auto entityHandle, auto& id) {
            Entity srcEntity = {entityHandle, other->m_Registry.get()};
            Entity dstEntity = newScene->CreateEntityWithUUID(id.ID);
            entityMap[entityHandle] = (entt::entity)dstEntity;

                if (srcRegistry.ctx().contains<ComponentSerializer*>())
                {
                    auto serializer = srcRegistry.ctx().get<ComponentSerializer*>();
                    if (serializer)
                        serializer->CopyAll(srcEntity, dstEntity);

                    // Reset physics handles so the specialized PhysicsSystem::InitializeBodies 
                    // in the new scene can create fresh native bodies for the runtime world.
                    if (dstEntity.HasComponent<RigidBodyComponent>())
                    {
                        dstEntity.GetComponent<RigidBodyComponent>().Handle = kInvalidPhysicsBody;
                    }
                }
        });
    }

    // Second pass: Copy and remap Hierarchy
    {
        CH_PROFILE_SCOPE("Scene::Copy::CopyEntities_Pass2");
        srcRegistry.view<HierarchyComponent>().each([&](auto entityHandle, auto& srcHC) {
            if (!entityMap.contains(entityHandle))
                return;

            entt::entity dstHandle = entityMap[entityHandle];
            auto& dstHC = dstRegistry.get_or_emplace<HierarchyComponent>(dstHandle);

            // Remap parent
            if (srcHC.Parent != entt::null && entityMap.count(srcHC.Parent))
            {
                dstHC.Parent = entityMap[srcHC.Parent];
                // Remove from roots if it has a parent
                auto& roots = newScene->GetRootEntities();
                auto it = std::find(roots.begin(), roots.end(), dstHandle);
                if (it != roots.end())
                    roots.erase(it);
            }
            else
            {
                dstHC.Parent = entt::null;
            }

            // Remap children
            dstHC.Children.clear();
            for (auto child : srcHC.Children)
            {
                if (entityMap.count(child))
                {
                    dstHC.Children.push_back(entityMap[child]);
                }
            }
        });

        CH_CORE_INFO("Scene::Copy - Pass 2 complete. Roots count: {0}", newScene->GetRootEntities().size());
    }

    CH_CORE_INFO("Scene::Copy - Finalizing copy ({} entities). Returning newScene pointer: {}", entityMap.size(), (void*)newScene.get());
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

    // 2. Remove from roots if it was a root
    auto& roots = reg.ctx().get<Scene*>()->GetRootEntities();
    auto it = std::find(roots.begin(), roots.end(), entity);
    if (it != roots.end())
    {
        roots.erase(it);
    }
}

void Scene::OnEvent(Event& e)
{
    m_ScriptingManager->OnEvent(e);
}

void Scene::OnRenderUI()
{
    m_ScriptingManager->OnRenderUI();
}

void Scene::OnRuntimeStart()
{
    CH_CORE_INFO("Scene::OnRuntimeStart - Starting activation for scene pointer: {}", (void*)this);
    Physics::ResetAccumulator(this);
    m_IsSimulationRunning = true;

    CH_CORE_INFO("Scene::OnRuntimeStart - Initializing script and system managers...");
    if (GetRegistry().ctx().contains<UIRenderer*>())
    {
        auto ui = GetRegistry().ctx().get<UIRenderer*>();
        if (ui)
            ui->ResetInputCooldown();
    }
    m_ScriptingManager->OnRuntimeStart();
    m_SystemManager->OnRuntimeStart();

    // Initialize PlayOnStart animations
    {
        auto& registry = GetRegistry();
        auto view = registry.view<AnimationComponent>();
        for (auto entity : view)
        {
            auto& anim = view.get<AnimationComponent>(entity);
            if (anim.PlayOnStart)
            {
                anim.IsPlaying = true;
            }
        }
    }

    CH_CORE_INFO("Scene::OnRuntimeStart - Activation complete.");
}

void Scene::OnRuntimeStop()
{
    m_IsSimulationRunning = false;

    m_ScriptingManager->OnRuntimeStop();
    m_SystemManager->OnRuntimeStop();
}



void Scene::OnUpdateRuntime(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    m_ScriptingManager->OnUpdate(timestep);

    m_SystemManager->OnUpdate(timestep);

    // Split animation responsibilities: playback
    AnimationSystems::UpdatePlayback(this, timestep);
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    m_SystemManager->OnUpdateEditor(timestep);
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

// Camera retrieval logic moved to SceneRenderer

// Primary camera entity retrieval moved to SceneRenderer helper logic

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

entt::registry* Scene::GetRegistryPtr()
{
    return m_Registry.get();
}
const entt::registry& Scene::GetRegistry() const
{
    return *m_Registry;
}
entt::registry& Scene::GetRegistry()
{
    return *m_Registry;
}
const SceneSettings& Scene::GetSettings() const
{
    return m_Settings;
}
SceneSettings& Scene::GetSettings()
{
    return m_Settings;
}
bool Scene::IsSimulationRunning() const
{
    return m_IsSimulationRunning;
}

ScriptEngine* Scene::GetScriptEngine() const
{
    return m_ScriptingManager ? m_ScriptingManager->GetScriptEngine() : nullptr;
}

void Scene::DestroyEntity(Entity entity)
{
    // Remove from root entities if it's there
    auto it = std::find(m_RootEntities.begin(), m_RootEntities.end(), (entt::entity)entity);
    if (it != m_RootEntities.end())
        m_RootEntities.erase(it);

    entity.Destroy();
}

Entity Scene::CopyEntity(entt::entity copyEntity)
{
    return CopyEntityInternal(copyEntity, entt::null);
}

Entity Scene::CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity)
{
    Entity srcEntity(copyEntity, m_Registry.get());
    std::string copyName = srcEntity.GetName() + (parentEntity == entt::null ? "_copy" : "");
    Entity dstEntity = CreateEntity(copyName);

    // CopyAll overwrites TagComponent and IDComponent with the source's values.
    // We must restore the copy's unique identity afterwards.
    if (GetRegistry().ctx().contains<ComponentSerializer*>())
    {
        auto serializer = GetRegistry().ctx().get<ComponentSerializer*>();
        if (serializer)
            serializer->CopyAll(srcEntity, dstEntity);
    }

    // Restore the name (CopyAll overwrites it with source tag)
    dstEntity.GetComponent<TagComponent>().Tag = copyName;

    // The IDComponent was copied verbatim from the source, so both entities now share
    // the same UUID. We must remove it and add a fresh one.
    dstEntity.RemoveComponent<IDComponent>();
    dstEntity.AddComponent<IDComponent>();

    // Handle Hierarchy
    if (srcEntity.HasComponent<HierarchyComponent>())
    {
        // CRITICAL: Copy srcHC to local variable. 
        // Subsequent AddOrReplaceComponent or Children.push_back calls on parent/children
        // will reallocate the HierarchyComponent pool, invalidating ANY reference.
        auto srcHC = srcEntity.GetComponent<HierarchyComponent>();

        entt::entity targetParent = parentEntity;
        if (targetParent == entt::null && srcHC.Parent != entt::null)
        {
            targetParent = srcHC.Parent;
        }

        if (targetParent != entt::null)
        {
            auto& dstHC = dstEntity.AddOrReplaceComponent<HierarchyComponent>();
            dstHC.Parent = targetParent;
            dstHC.Children.clear();

            Entity parent(targetParent, m_Registry.get());
            if (parent.HasComponent<HierarchyComponent>())
            {
                parent.GetComponent<HierarchyComponent>().Children.push_back(dstEntity);
            }
        }
        else
        {
            if (dstEntity.HasComponent<HierarchyComponent>())
            {
                dstEntity.GetComponent<HierarchyComponent>().Parent = entt::null;
                dstEntity.GetComponent<HierarchyComponent>().Children.clear();
            }
        }

        // Recursively copy all children using the local copy of the children list
        for (auto child : srcHC.Children)
        {
            CopyEntityInternal(child, dstEntity);
        }
    }

    return dstEntity;
}

Entity Scene::CreateUIEntity(const std::string& type, const std::string& name)
{
    Entity entity = CreateEntity(name.empty() ? type : name);
    entity.AddComponent<ControlComponent>();

    UIFactory::Create(type, entity);

    return entity;
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry.get());
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    
    m_RootEntities.push_back(entity);
    return entity;
}

Entity Scene::CreateEntity(const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry.get());
    entity.AddComponent<IDComponent>();
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    
    m_RootEntities.push_back(entity);
    return entity;
}

Entity Scene::FindEntityByTag(const std::string& tag)
{
    auto view = m_Registry->view<TagComponent>();
    for (auto entity : view)
    {
        const auto& tagComp = view.get<TagComponent>(entity);
        if (tagComp.Tag == tag)
        {
            return {entity, m_Registry.get()};
        }
    }
    return {};
}

Entity Scene::GetEntityByUUID(UUID uuid)
{
    auto* mapStruct = m_Registry->ctx().find<EntityUUIDMap>();
    if (mapStruct && mapStruct->Map.find(uuid) != mapStruct->Map.end())
    {
        return {mapStruct->Map.at(uuid), m_Registry.get()};
    }
    return {};
}
} // namespace CHEngine
