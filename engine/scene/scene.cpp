#include "engine/scene/scene.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/systems/animation_system.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/scene/systems/scene_audio_system.h"
#include "SceneScriptingManager.h"
#include <entt/entt.hpp>
#include <glm/gtx/norm.hpp>

using namespace entt::literals;

namespace CHEngine
{
// Scene implementation
Scene::Scene()
{
    // Create registry
    m_Registry = std::make_shared<entt::registry>();
    m_ScriptingManager = std::make_unique<SceneScriptingManager>(this);

    auto& reg = *m_Registry;
    reg.ctx().emplace<Scene*>(this);
    reg.ctx().emplace<EntityUUIDMap>();
    reg.ctx().emplace<std::shared_ptr<entt::registry>>(m_Registry);

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
    Physics::ClearContext(this);
    // Clean up active signals
    GetRegistry().clear();
    
    // Break circular dependency to allow registry to be safely deallocated
    m_Registry->ctx().erase<std::shared_ptr<entt::registry>>();
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

    // 2. Copy Entities (Direct Memory Copy)
    auto& srcRegistry = other->GetRegistry();
    auto& dstRegistry = newScene->GetRegistry();

    // Copy all entities using ComponentSerializer
    {
        CH_PROFILE_SCOPE("Scene::Copy::CopyEntities");
        int entityCount = 0;
        srcRegistry.view<IDComponent>().each([&](auto entityHandle, auto& id) {
            entityCount++;
            Entity srcEntity = {entityHandle, other->m_Registry};
            Entity dstEntity = newScene->CreateEntityWithUUID(id.ID);

            ComponentSerializer::Get().CopyAll(srcEntity, dstEntity);
        });

        CH_CORE_INFO("Scene::Copy - Successfully copied {} entities", entityCount);
    }
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
    Physics::ResetAccumulator(this);
    m_IsSimulationRunning = true;

    m_ScriptingManager->OnRuntimeStart();
}

void Scene::OnRuntimeStop()
{
    m_IsSimulationRunning = false;

    m_ScriptingManager->OnRuntimeStop();
    Physics::ClearContext(this);
}

void Scene::OnUpdateRuntime(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    m_ScriptingManager->OnUpdate(timestep);

    HierarchySystem::Update(this);
    AnimationSystem::Update(this, timestep);
    Physics::Update(this, timestep, m_IsSimulationRunning);
    SceneAudioSystem::Update(this, timestep);
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    HierarchySystem::Update(this);
    AnimationSystem::Update(this, timestep);
    Physics::Update(this, timestep, false);
    SceneAudioSystem::Update(this, timestep);
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
            return {entity, m_Registry};
        }
    }
    return {};
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

std::shared_ptr<entt::registry> Scene::GetRegistryPtr()
{
    return m_Registry;
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
void Scene::DestroyEntity(Entity entity)
{
    entity.Destroy();
}

Entity Scene::CopyEntity(entt::entity copyEntity)
{
    return CopyEntityInternal(copyEntity, entt::null);
}

Entity Scene::CopyEntityInternal(entt::entity copyEntity, entt::entity parentEntity)
{
    Entity srcEntity(copyEntity, m_Registry);
    std::string copyName = srcEntity.GetName() + (parentEntity == entt::null ? "_copy" : "");
    Entity dstEntity = CreateEntity(copyName);

    // CopyAll overwrites TagComponent and IDComponent with the source's values.
    // We must restore the copy's unique identity afterwards.
    ComponentSerializer::Get().CopyAll(srcEntity, dstEntity);

    // Restore the name (CopyAll overwrites it with source tag)
    dstEntity.GetComponent<TagComponent>().Tag = copyName;

    // The IDComponent was copied verbatim from the source, so both entities now share
    // the same UUID. We must remove it and add a fresh one.
    dstEntity.RemoveComponent<IDComponent>();
    dstEntity.AddComponent<IDComponent>(); 

    // Handle Hierarchy
    if (srcEntity.HasComponent<HierarchyComponent>())
    {
        auto& srcHC = srcEntity.GetComponent<HierarchyComponent>();
        
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

            Entity parent(targetParent, m_Registry);
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

        // Recursively copy all children
        std::vector<entt::entity> childrenToCopy = srcHC.Children;
        for (auto child : childrenToCopy)
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
    
    if (type == "Button")                       entity.AddComponent<ButtonControl>();
    else if (type == "Panel")                  entity.AddComponent<PanelControl>();
    else if (type == "Label")                  entity.AddComponent<LabelControl>();
    else if (type == "Slider")                 entity.AddComponent<SliderControl>();
    else if (type == "CheckBox")               entity.AddComponent<CheckboxControl>();
    else if (type == "InputText")              entity.AddComponent<InputTextControl>();
    else if (type == "ComboBox")               entity.AddComponent<ComboBoxControl>();
    else if (type == "ProgressBar")            entity.AddComponent<ProgressBarControl>();
    else if (type == "Image")                  entity.AddComponent<ImageControl>();
    else if (type == "ImageButton")            entity.AddComponent<ImageButtonControl>();
    else if (type == "Separator")              entity.AddComponent<SeparatorControl>();
    else if (type == "RadioButton")            entity.AddComponent<RadioButtonControl>();
    else if (type == "ColorPicker")            entity.AddComponent<ColorPickerControl>();
    else if (type == "DragFloat")              entity.AddComponent<DragFloatControl>();
    else if (type == "DragInt")                entity.AddComponent<DragIntControl>();
    else if (type == "TreeNode")               entity.AddComponent<TreeNodeControl>();
    else if (type == "TabBar")                 entity.AddComponent<TabBarControl>();
    else if (type == "TabItem")                entity.AddComponent<TabItemControl>();
    else if (type == "CollapsingHeader")       entity.AddComponent<CollapsingHeaderControl>();
    else if (type == "PlotLines")              entity.AddComponent<PlotLinesControl>();
    else if (type == "PlotHistogram")          entity.AddComponent<PlotHistogramControl>();
    else if (type == "VerticalLayoutGroup")    entity.AddComponent<VerticalLayoutGroup>();
    else if (type == "UIAction")               entity.AddComponent<UIActionComponent>();

    return entity;
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry);
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();
    return entity;
}

Entity Scene::CreateEntity(const std::string& name)
{
    Entity entity(m_Registry->create(), m_Registry);
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
            return {entity, m_Registry};
        }
    }
    return {};
}

Entity Scene::GetEntityByUUID(UUID uuid)
{
    auto* mapStruct = m_Registry->ctx().find<EntityUUIDMap>();
    if (mapStruct && mapStruct->Map.find(uuid) != mapStruct->Map.end())
    {
        return {mapStruct->Map.at(uuid), m_Registry};
    }
    return {};
}
} // namespace CHEngine
