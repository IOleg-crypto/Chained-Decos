// scene.cpp
// Chained Engine — ECS scene management and update pipeline.
// Coordinates entity lifecycle, hierarchy, scripting, physics, animation, and scene transitions.

#include "engine/scene/scene.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/ui/ui_factory.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/scene_transition_component.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/scene/systems/scene_resource_manager.h"
#include "engine/scene/component_serializer.h"
#include "scene_scripting_manager.h"
#include "scripting/scriptengine.h"
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <thread>
#include "engine/common/thread_pool.h"

using namespace entt::literals;

namespace Chained
{
Scene::Scene()
{
    m_Registry = std::make_unique<entt::registry>();
    auto& reg = *m_Registry;

    reg.ctx().emplace<Scene*>(this);
    reg.ctx().emplace<EntityUUIDMap>();

    reg.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    reg.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    SceneResources::RegisterObservers(reg);

    m_Settings.Environment = std::make_shared<EnvironmentAsset>();
    m_ScriptingManager = std::make_unique<SceneScriptingManager>(this);

    UIFactory::Initialize();
}

Scene::~Scene()
{
    GetRegistry().clear();
}

std::shared_ptr<Scene> Scene::CreateDefault()
{
    auto scene = std::make_shared<Scene>();

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
    newScene->m_Settings = other->m_Settings;

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

            ComponentSerializer::CopyAll(srcEntity, dstEntity);

            if (dstEntity.HasComponent<RigidBodyComponent>())
            {
                dstEntity.GetComponent<RigidBodyComponent>().Handle = kInvalidPhysicsBody;
            }
        }
    }

    // Second pass: Copy hierarchy relationships
    {
        CH_PROFILE_SCOPE("Scene::Copy::CopyEntities_Pass2");
        srcRegistry.view<HierarchyComponent>().each([&](auto entityHandle, auto& srcHC) {
            if (!entityMap.contains(entityHandle))
            {
                return;
            }

            entt::entity dstHandle = entityMap[entityHandle];
            auto& dstHC = dstRegistry.get_or_emplace<HierarchyComponent>(dstHandle);

            if (srcHC.Parent != entt::null && entityMap.count(srcHC.Parent))
            {
                dstHC.Parent = entityMap[srcHC.Parent];
            }
            else
            {
                dstHC.Parent = entt::null;
            }

            dstHC.Children.clear();
            for (auto child : srcHC.Children)
            {
                if (entityMap.count(child))
                {
                    dstHC.Children.push_back(entityMap[child]);
                }
            }
        });
    }

    return newScene;
}

void Scene::OnHierarchyDestroy(entt::registry& reg, entt::entity entity)
{
    auto& hc = reg.get<HierarchyComponent>(entity);

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
    // Scripts receive events only during full gameplay
    if (m_State == SceneState::Play && !m_IsStartingUp)
    {
        m_ScriptingManager->OnEvent(e);
    }
}

void Scene::OnRenderUI()
{
    if (m_State == SceneState::Play && !m_IsStartingUp)
    {
        m_ScriptingManager->OnRenderUI();
    }
}

void Scene::OnRuntimeStart(const SceneContext& ctx)
{
    CH_CORE_INFO("Scene::OnRuntimeStart - Starting activation for state: {}", (int)m_State);

    // Cache the context on the registry so code that only has an entt::registry&
    // (e.g. SceneResources::OnRigidBodyConstruct, invoked with a fixed EnTT callback
    // signature) can still reach these services without going through ServiceLocator.
    m_Registry->ctx().insert_or_assign<SceneContext>(SceneContext(ctx));

    m_IsStartingUp = true;

    // We defer physics initialization to the next frame in OnUpdateRuntime
    // so that the engine has a chance to render the "Loading..." overlay once!
}

void Scene::FinishRuntimeStart(const SceneContext& ctx)
{
    auto* scripting = ctx.Scripting ? ctx.Scripting : ServiceLocator::TryGet<ScriptEngine>();
    if (scripting)
    {
        scripting->SetContextScene(this);
    }

    // Start scripts only when in full gameplay state
    if (m_State == SceneState::Play)
    {
        if (m_ScriptingManager)
        {
            m_ScriptingManager->OnRuntimeStart();
        }
        else
        {
            CH_CORE_ERROR("Scene::OnRuntimeStart - m_ScriptingManager is NULL!");
        }
    }

    SceneResources::OnRuntimeStart(this);

    // Reset any stale SceneTransitionComponent::Triggered flags that may have been
    // serialized as `true` in the scene file — transitions should only fire on user input.
    {
        auto transitionView = m_Registry->view<SceneTransitionComponent>();
        for (auto entity : transitionView)
        {
            transitionView.get<SceneTransitionComponent>(entity).Triggered = false;
        }
    }

    // Start animations
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

void Scene::OnRuntimeStop(const SceneContext& ctx)
{
    CH_CORE_INFO("Scene::OnRuntimeStop - Stopping lifecycle...");

    m_IsStartingUp = false;

    if (m_ScriptingManager)
    {
        m_ScriptingManager->OnRuntimeStop();
    }

    SceneResources::OnRuntimeStop(this);

    auto* physics = ctx.PhysicsSystem ? ctx.PhysicsSystem : ServiceLocator::TryGet<Physics>();
    if (physics)
    {
        physics->ClearContext(this);
    }

    auto* scripting = ctx.Scripting ? ctx.Scripting : ServiceLocator::TryGet<ScriptEngine>();
    if (scripting)
    {
        scripting->SetContextScene(nullptr);
    }

    m_Registry->ctx().erase<SceneContext>();
}

void Scene::OnUpdateRuntime(Timestep ts, const SceneContext& ctx)
{
    CH_PROFILE_FUNCTION();

    auto* physics = ctx.PhysicsSystem ? ctx.PhysicsSystem : ServiceLocator::TryGet<Physics>();

    if (m_IsStartingUp)
    {
        if (physics)
        {
            physics->ResetWorld(this);
            physics->ResetAccumulator(this);
            physics->InitializeBodies(this);
        }

        m_IsStartingUp = false;
        FinishRuntimeStart(ctx);
        return;
    }

    if (m_ScriptingManager)
    {
        m_ScriptingManager->OnUpdate(ts);
    }

    // 1. Hierarchy Update
    Hierarchy::UpdateWorldTransforms(*m_Registry, GetRootEntities());

    // 2. Resource & Asset Resolution
    SceneResources::Update(*m_Registry, ts);

    // 3. Physics Simulation
    if (physics)
    {
        physics->Update(this, ts, true);
    }

    // 4. Animation Playback (handled by SceneResources::Update)

    // 5. Scene Transitions
    auto transitionView = m_Registry->view<SceneTransitionComponent>();
    for (auto entity : transitionView)
    {
        auto& transition = transitionView.get<SceneTransitionComponent>(entity);
        if (m_Registry->all_of<UIControlComponent>(entity))
        {
            auto& widget = m_Registry->get<UIControlComponent>(entity);
            if (widget.PressedThisFrame)
            {
                transition.Triggered = true;
            }
        }

        if (transition.Triggered && !transition.TargetScenePath.empty())
        {
            SceneChangeRequestEvent ev(transition.TargetScenePath);
            if (m_EventCallback)
            {
                m_EventCallback(ev);
            }
            else
            {
                CH_CORE_WARN("Scene transition triggered but no EventCallback bound!");
            }

            transition.Triggered = false;
        }
    }
}

void Scene::OnUpdateSimulation(Timestep ts, const SceneContext& ctx)
{
    CH_PROFILE_FUNCTION();

    auto* physics = ctx.PhysicsSystem ? ctx.PhysicsSystem : ServiceLocator::TryGet<Physics>();

    if (m_IsStartingUp)
    {
        // Synchronous initialization on the main thread (blocks, but loading screen is visible)
        if (physics)
        {
            physics->ResetWorld(this);
            physics->ResetAccumulator(this);
            physics->InitializeBodies(this);
        }

        m_IsStartingUp = false;
        FinishRuntimeStart(ctx);
        return;
    }

    // 1. Hierarchy Update
    Hierarchy::UpdateWorldTransforms(*m_Registry, GetRootEntities());

    // 2. Resource & Asset Resolution
    SceneResources::Update(*m_Registry, ts);

    // 3. Physics Simulation
    if (physics)
    {
        physics->Update(this, ts, true);
    }

    // 4. Animation Playback (handled by SceneResources::Update)
}

void Scene::OnUpdateEditor(Timestep timestep, const SceneContext& ctx)
{
    CH_PROFILE_FUNCTION();

    Hierarchy::UpdateWorldTransforms(*m_Registry, GetRootEntities());

    if (ctx.PhysicsSystem)
    {
        ctx.PhysicsSystem->Update(this, timestep, false);
    }

    SceneResources::Update(*m_Registry, timestep);
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

std::vector<entt::entity> Scene::GetRootEntities()
{
    return GetRootEntitiesImpl();
}

std::vector<entt::entity> Scene::GetRootEntities() const
{
    return GetRootEntitiesImpl();
}

std::vector<entt::entity> Scene::GetRootEntitiesImpl() const
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
            {
                roots.push_back(entity);
            }
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
    return m_State == SceneState::Play || m_State == SceneState::Simulate;
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

    ComponentSerializer::CopyAll(srcEntity, dstEntity);
    dstEntity.GetComponent<TagComponent>().Tag = copyName;

    dstEntity.RemoveComponent<IDComponent>();
    dstEntity.AddComponent<IDComponent>();

    if (dstEntity.HasComponent<RigidBodyComponent>())
    {
        dstEntity.GetComponent<RigidBodyComponent>().Handle = kInvalidPhysicsBody;
    }

    if (srcEntity.HasComponent<HierarchyComponent>())
    {
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

void Scene::TransitionToState(SceneState newState, const SceneContext& ctx)
{
    if (m_State == newState)
    {
        return;
    }

    OnStateExit(m_State, ctx);
    SceneState oldState = m_State;
    m_State = newState;
    OnStateEnter(m_State, ctx);

    CH_CORE_TRACE("Scene: Transitioned state from {} to {}", (int)oldState, (int)newState);
}

void Scene::OnStateEnter(SceneState state, const SceneContext& ctx)
{
    switch (state)
    {
    case SceneState::Play:
    case SceneState::Simulate:
        OnRuntimeStart(ctx);
        break;
    case SceneState::Edit:
        break;
    }
}

void Scene::OnStateExit(SceneState state, const SceneContext& ctx)
{
    switch (state)
    {
    case SceneState::Play:
    case SceneState::Simulate:
        OnRuntimeStop(ctx);
        break;
    case SceneState::Edit:
        break;
    }
}

void Scene::OnUpdate(Timestep timestep, const SceneContext& ctx)
{
    switch (m_State)
    {
    case SceneState::Edit:
        OnUpdateEditor(timestep, ctx);
        break;
    case SceneState::Play:
        OnUpdateRuntime(timestep, ctx);
        break;
    case SceneState::Simulate:
        OnUpdateSimulation(timestep, ctx);
        break;
    }
}
} // namespace Chained