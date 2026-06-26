#include "engine/scene/scene.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/scene/systems/scene_resource_manager.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/animation_systems.h"
#include "engine/scene/components/scene_transition_component.h"
#include "engine/scene/components/control_component.h"
#include "engine/graphics/ui/ui_renderer.h"
#include <yaml-cpp/yaml.h>
#include "engine/graphics/ui/ui_factory.h"
#include "scene_scripting_manager.h"
#include "scripting/scriptengine.h"
#include "engine/serialization/component_serializer.h"
#include <entt/entt.hpp>
#include "engine/core/service_locator.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/graphics/pipeline/renderer.h"

using namespace entt::literals;


namespace Chained
{
// Scene implementation
Scene::Scene()
{
    // Create registry
    m_Registry = std::make_unique<entt::registry>();
    auto& reg = *m_Registry;
    
    // Populate Context
    reg.ctx().emplace<Scene*>(this);
    reg.ctx().emplace<EntityUUIDMap>();

    m_HierarchySystem = std::make_unique<HierarchySystem>();
    m_ResourceManager = std::make_unique<SceneResourceManager>();
    m_AnimationManager = std::make_unique<AnimationManager>();

    // Register Resource Observers
    if (m_ResourceManager)
        m_ResourceManager->RegisterObservers(reg);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();

    m_ScriptingManager = std::make_unique<SceneScriptingManager>(this);
    
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
     
    auto scene = std::make_shared<Scene>();

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

    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // 1. Copy Scene Settings
    newScene->m_Settings = other->m_Settings;

    // 2. Copy Entities (remap handled via ID)
    auto& srcRegistry = other->GetRegistry();
    auto& dstRegistry = newScene->GetRegistry();

    std::unordered_map<entt::entity, entt::entity> entityMap;

    // First pass: Create all entities and copy basic components
    {
        CH_PROFILE_SCOPE("Scene::Copy::CopyEntities_Pass1");
        for (auto entityHandle : srcRegistry.view<IDComponent>())
        {
            auto& id = srcRegistry.get<IDComponent>(entityHandle);
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
        }
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

        CH_CORE_INFO("Scene::Copy - Pass 2 complete.");
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
    if (auto* uiRenderer = ServiceLocator::Get<Renderer>()->GetUIRenderer())
        uiRenderer->ResetInputCooldown();
    
    if (m_ScriptingManager)
        m_ScriptingManager->OnRuntimeStart();
    else
        CH_CORE_ERROR("Scene::OnRuntimeStart - m_ScriptingManager is NULL!");

    if (m_ResourceManager)
        m_ResourceManager->OnRuntimeStart(this);

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
    
    if (m_ResourceManager)
        m_ResourceManager->OnRuntimeStop(this);
}

void Scene::OnUpdateRuntime(Timestep ts)
{
    CH_PROFILE_FUNCTION();
 
    if (m_ScriptingManager)
        m_ScriptingManager->OnUpdate(ts);
 
    // 1. Hierarchy Update
    if (m_HierarchySystem)
        m_HierarchySystem->UpdateWorldTransforms(*m_Registry, GetRootEntities());

    // 2. Resource & Asset Resolution
    if (m_ResourceManager)
        m_ResourceManager->Update(*m_Registry, ts);

    // 3. Physics Simulation
    Physics::Update(this, ts, true);

    // 4. Animation Playback
    if (m_AnimationManager)
        m_AnimationManager->UpdatePlayback(this, ts);

    // 5. Scene Transitions
    auto transitionView = m_Registry->view<SceneTransitionComponent>();
    for (auto entity : transitionView)
    {
        auto& transition = transitionView.get<SceneTransitionComponent>(entity);
        if (m_Registry->all_of<UIControlComponent>(entity))
        {
            auto& widget = m_Registry->get<UIControlComponent>(entity);
            if (widget.PressedThisFrame) transition.Triggered = true;
        }

        if (transition.Triggered && !transition.TargetScenePath.empty())
        {
            SceneChangeRequestEvent ev(transition.TargetScenePath);
            if (m_EventCallback)
                m_EventCallback(ev);
            else
                CH_CORE_WARN("Scene transition triggered but no EventCallback bound!");
            
            transition.Triggered = false;
        }
    }
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    // 1. Hierarchy Update
    m_HierarchySystem->UpdateWorldTransforms(*m_Registry, GetRootEntities());

    // 2. Resource & Asset Resolution (Editor needs this for lazy loading too)
    if (m_ResourceManager)
        m_ResourceManager->Update(*m_Registry, timestep);
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
            // cameraComponent.Camera.SetViewportSize(width, height); // No explicit SetViewportSize, let rendering handle aspect ratio
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

std::vector<entt::entity> Chained::Scene::GetRootEntities()
{
    std::vector<entt::entity> roots;
    auto& reg = GetRegistry();

    auto view = reg.template view<TransformComponent>();
    for (auto entity : view)
    {
        if (reg.template all_of<HierarchyComponent>(entity))
        {
            auto& hc = reg.template get<HierarchyComponent>(entity);
            if (hc.Parent == entt::null)
                roots.push_back(entity);
        }
        else
        {
            roots.push_back(entity);
        }
    }
    return roots;
}

std::vector<entt::entity> Chained::Scene::GetRootEntities() const
{
    std::vector<entt::entity> roots;
    auto& reg = GetRegistry();

    auto view = reg.template view<TransformComponent>();
    for (auto entity : view)
    {
        if (reg.template all_of<HierarchyComponent>(entity))
        {
            auto& hc = reg.template get<HierarchyComponent>(entity);
            if (hc.Parent == entt::null)
                roots.push_back(entity);
        }
        else
        {
            roots.push_back(entity);
        }
    }
    return roots;
}
bool Scene::IsSimulationRunning() const
{
    return m_IsSimulationRunning;
}

void Scene::DestroyEntity(Entity entity)
{
    entity.Destroy();
}

entt::entity Scene::CopyEntity(entt::entity copyEntity)
{
    return (entt::entity)CopyEntityInternal(copyEntity, entt::null);
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
    
    return entity;
}

Entity Scene::CreateEntity(const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry.get());
    entity.AddComponent<IDComponent>();
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    
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
} // namespace Chained
