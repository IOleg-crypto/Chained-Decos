#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/render.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene_scripting.h"
#include "project.h"
#include "raylib.h"
#include "raymath.h"
#include "scene_serializer.h"
#include "scriptable_entity.h"
#include "engine/scene/scene_events.h"

namespace CHEngine
{
void Scene::RequestSceneChange(const std::string &path)
{
    // Decoupled transition request via Event System
    SceneChangeRequestEvent e(path);
    Application::OnEvent(e);
}
Scene::Scene()
{
    // Declarative signals binding
    m_Registry.on_construct<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);
    m_Registry.on_update<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);

    m_Registry.on_construct<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);
    m_Registry.on_update<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();
    
    // UUID Mapping
    m_Registry.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    m_Registry.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    // Hierarchy Mapping
    m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyDestroy>(this);

    // Create physics instance
    m_Physics = std::make_unique<Physics>(this);
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    SceneSerializer serializer(other.get());
    std::string yaml = serializer.SerializeToString();

    SceneSerializer deserializer(newScene.get());
    if (deserializer.DeserializeFromString(yaml))
    {
        return newScene;
    }

    return nullptr;
}

Entity Scene::CreateEntity(const std::string &name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<IDComponent>();
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created: {} ({})", name, (uint64_t)entity.GetComponent<IDComponent>().ID);
    return entity;
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created with UUID: {} ({})", name, (uint64_t)uuid);
    return entity;
}

Entity Scene::CreateUIEntity(const std::string &type, const std::string &name)
{
    Entity entity = CreateEntity(name.empty() ? type : name);
    entity.AddComponent<ControlComponent>();

    if (type == "Button")
    {
        entity.AddComponent<ButtonControl>();
    }
    else if (type == "Panel")
    {
        entity.AddComponent<PanelControl>();
    }
    else if (type == "Label")
    {
        entity.AddComponent<LabelControl>();
    }
    else if (type == "Slider")
    {
        entity.AddComponent<SliderControl>();
    }
    else if (type == "CheckBox")
    {
        entity.AddComponent<CheckboxControl>();
    }

    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    CH_CORE_ASSERT(entity, "Entity is null!");
    
    // Recursive destruction
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto children = entity.GetComponent<HierarchyComponent>().Children;
        for (auto childHandle : children)
            DestroyEntity({childHandle, this});
    }

    CH_CORE_INFO("Entity Destroyed: {} ({})", entity.GetName(), (uint32_t)entity);
    m_Registry.destroy(entity);
}

void Scene::OnHierarchyDestroy(entt::registry &reg, entt::entity entity)
{
    auto &hc = reg.get<HierarchyComponent>(entity);

    // 1. Detach from parent
    if (hc.Parent != entt::null && reg.valid(hc.Parent) &&
        reg.all_of<HierarchyComponent>(hc.Parent))
    {
        auto &phc = reg.get<HierarchyComponent>(hc.Parent);
        auto it = std::find(phc.Children.begin(), phc.Children.end(), entity);
        if (it != phc.Children.end())
        {
            phc.Children.erase(it);
        }
    }

    // Children are handled by recursive DestroyEntity call
}

template <> void Scene::OnComponentAdded<ModelComponent>(Entity entity, ModelComponent &component)
{
    if (!component.ModelPath.empty())
    {
        // Check if we need to (re)load the asset
        bool pathChanged = !component.Asset || (component.Asset->GetPath() != component.ModelPath);

        if (pathChanged)
        {
            auto project = Project::GetActive();
            if (project && project->GetAssetManager())
            {
                component.Asset = project->GetAssetManager()->Get<ModelAsset>(component.ModelPath);
            }
            component.MaterialsInitialized = false; 
        }

        if (component.Asset && !component.MaterialsInitialized)
        {
            Model &model = component.Asset->GetModel();
            component.Materials.clear();
            
            if (model.materials != nullptr)
            {
                std::set<int> uniqueIndices;
                for (int i = 0; i < model.meshCount; i++)
                {
                    uniqueIndices.insert(model.meshMaterial[i]);
                }

                for (int idx : uniqueIndices)
                {
                    if (idx >= 0 && idx < model.materialCount)
                    {
                        MaterialSlot slot("Material " + std::to_string(idx), idx);
                        slot.Target = MaterialSlotTarget::MaterialIndex;
                        slot.Material.AlbedoColor = model.materials[idx].maps[MATERIAL_MAP_ALBEDO].color;
                        component.Materials.push_back(slot);
                    }
                }
            }
            component.MaterialsInitialized = true;
        }
    }
}

template <>
void Scene::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent &component)
{
    // Animation component might need the model asset to perform calculations
    if (entity.HasComponent<ModelComponent>())
    {
        auto &mc = entity.GetComponent<ModelComponent>();
        if (!mc.Asset && !mc.ModelPath.empty())
        {
            auto project = Project::GetActive();
            if (project && project->GetAssetManager())
            {
                mc.Asset = project->GetAssetManager()->Get<ModelAsset>(mc.ModelPath);
            }
        }
    }
}

template <> void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent &component)
{
    if (!component.SoundPath.empty())
    {
        auto project = Project::GetActive();
        if (project && project->GetAssetManager())
        {
            component.Asset = project->GetAssetManager()->Get<SoundAsset>(component.SoundPath);
        }
    }
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    CH_PROFILE_FUNCTION();

    bool isSim = IsSimulationRunning();
    m_Physics->Update(deltaTime, isSim);

    SceneScripting::Update(this, deltaTime);
    Audio::Update(this, deltaTime);

    // Declarative Scene Transitions (Reactive Logic)
    m_Registry.view<SceneTransitionComponent>().each([&](auto entity, auto& tr) {
        if (tr.Triggered && !tr.TargetScenePath.empty())
            RequestSceneChange(tr.TargetScenePath);
    });
}


// Logic moved to Render phase (Declarative Posing)

void Scene::OnRender(const Camera3D &camera, Timestep ts, const DebugRenderFlags *debugFlags)
{
    CH_PROFILE_FUNCTION();

    // Late initialization for async loaded models
    auto view = m_Registry.view<ModelComponent>();
    for (auto entityID : view)
    {
        auto &mc = view.get<ModelComponent>(entityID);
        if (mc.Asset && mc.Asset->IsReady() && !mc.MaterialsInitialized)
        {
            Entity entity = {entityID, this};
            OnComponentAdded<ModelComponent>(entity, mc);
        }
    }

    Render::DrawScene(this, camera, ts, debugFlags);
}

Camera3D Scene::GetActiveCamera() const
{
    // First: Check for primary camera entity
    auto cameraView = m_Registry.view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto &cam = cameraView.get<CameraComponent>(entity);
        if (cam.IsActive && cam.IsPrimary)
        {
            auto &transform = cameraView.get<TransformComponent>(entity);
            
            Camera3D camera = {0};
            camera.position = transform.Translation;
            
            // Calculate target from rotation
            float yaw = transform.Rotation.y;
            float pitch = transform.Rotation.x;
            camera.target = {
                transform.Translation.x - sinf(yaw) * cosf(pitch),
                transform.Translation.y + sinf(pitch),
                transform.Translation.z - cosf(yaw) * cosf(pitch)
            };
            
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = cam.Fov;
            camera.projection = cam.Projection == 0 ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
            return camera;
        }
    }

    // Fallback: Use PlayerComponent third-person camera
    auto view = m_Registry.view<PlayerComponent, TransformComponent>();
    if (view.begin() != view.end())
    {
        auto entity = *view.begin();
        auto &transform = view.get<TransformComponent>(entity);
        auto &player = view.get<PlayerComponent>(entity);

        Vector3 target = transform.Translation;
        target.y += 1.0f;

        float yawRad = player.CameraYaw * DEG2RAD;
        float pitchRad = player.CameraPitch * DEG2RAD;

        Vector3 offset;
        offset.x = player.CameraDistance * cosf(pitchRad) * sinf(yawRad);
        offset.y = player.CameraDistance * sinf(pitchRad);
        offset.z = player.CameraDistance * cosf(pitchRad) * cosf(yawRad);

        Camera3D camera = {0};
        camera.position = Vector3Add(target, offset);
        camera.target = target;
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 90.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

    // Default fallback
    Camera3D camera = {0};
    camera.position = {10.0f, 10.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

EnvironmentSettings Scene::GetEnvironmentSettings() const
{
    if (m_Settings.Environment)
        return m_Settings.Environment->GetSettings();

    return EnvironmentSettings();
}

// -------------------------------------------------------------------------------------------------------------------

void Scene::OnRuntimeStart()
{
    m_IsSimulationRunning = true;
    CH_CORE_INFO("Scene '{}' simulation started.",
                 m_Registry.view<TagComponent>()
                     .get<TagComponent>(m_Registry.view<TagComponent>().front())
                     .Tag);
}

void Scene::OnRuntimeStop()
{
    m_IsSimulationRunning = false;
    // TODO: Cleanup runtime state
}

void Scene::OnEvent(Event &e)
{
    SceneScripting::DispatchEvent(this, e);
}

void Scene::OnImGuiRender(const ImVec2 &refPos, const ImVec2 &refSize, uint32_t viewportID,
                          bool editMode)
{
    UpdateProfilerStats();
    
    // Delegate complex UI logic to Render action class
    Render::DrawUI(this, refPos, refSize, editMode);

    // Scripted UI elements (legacy support)
    //SceneScripting::RenderUI(this);
}

Entity Scene::FindEntityByTag(const std::string &tag)
{
    auto view = m_Registry.view<TagComponent>();
    for (auto entity : view)
    {
        const auto &tagComp = view.get<TagComponent>(entity);
        if (tagComp.Tag == tag)
            return {entity, this};
    }
    return {};
}
Entity Scene::GetEntityByUUID(UUID uuid)
{
    if (m_EntityMap.find(uuid) != m_EntityMap.end())
        return {m_EntityMap.at(uuid), this};
    return {};
}

void Scene::OnIDConstruct(entt::registry &reg, entt::entity entity)
{
    auto &id = reg.get<IDComponent>(entity).ID;
    m_EntityMap[id] = entity;
}

void Scene::OnIDDestroy(entt::registry &reg, entt::entity entity)
{
    auto &id = reg.get<IDComponent>(entity).ID;
    m_EntityMap.erase(id);
}

void Scene::OnAudioComponentAdded(entt::registry &reg, entt::entity entity)
{
    auto &audio = reg.get<AudioComponent>(entity);
    OnComponentAdded<AudioComponent>(Entity{entity, this}, audio);
    
    // Reactive Sound Playback
    if (audio.PlayOnStart && !audio.IsPlaying) {
        audio.IsPlaying = true;
        if (audio.Asset) Audio::Play(audio.Asset, audio.Volume, audio.Pitch, audio.Loop);
    }
}

void Scene::OnModelComponentAdded(entt::registry &reg, entt::entity entity)
{
    OnComponentAdded<ModelComponent>(Entity{entity, this}, reg.get<ModelComponent>(entity));
}

void Scene::OnAnimationComponentAdded(entt::registry &reg, entt::entity entity)
{
    OnComponentAdded<AnimationComponent>(Entity{entity, this}, reg.get<AnimationComponent>(entity));
}

void Scene::UpdateProfilerStats()
{
    CH_PROFILE_FUNCTION();

    ::CHEngine::ProfilerStats stats;
    stats.EntityCount = (uint32_t)m_Registry.storage<entt::entity>().size();

    auto view = m_Registry.view<::CHEngine::ColliderComponent>();
    stats.ColliderCount = (uint32_t)view.size();

    ::CHEngine::Profiler::UpdateStats(stats);
}

BackgroundMode Scene::GetBackgroundMode() const { return m_Settings.Mode; }
void Scene::SetBackgroundMode(BackgroundMode mode) { m_Settings.Mode = mode; }

Color Scene::GetBackgroundColor() const { return m_Settings.BackgroundColor; }
void Scene::SetBackgroundColor(Color color) { m_Settings.BackgroundColor = color; }

const std::string& Scene::GetBackgroundTexturePath() const { return m_Settings.BackgroundTexturePath; }
void Scene::SetBackgroundTexturePath(const std::string& path) { m_Settings.BackgroundTexturePath = path; }

bool Scene::IsSimulationRunning() const { return m_IsSimulationRunning; }

const std::string& Scene::GetScenePath() const { return m_Settings.ScenePath; }
void Scene::SetScenePath(const std::string& path) { m_Settings.ScenePath = path; }

std::shared_ptr<EnvironmentAsset> Scene::GetEnvironment() { return m_Settings.Environment; }
const std::shared_ptr<EnvironmentAsset> Scene::GetEnvironment() const { return m_Settings.Environment; }
void Scene::SetEnvironment(std::shared_ptr<EnvironmentAsset> environment) { m_Settings.Environment = environment; }

CanvasSettings& Scene::GetCanvasSettings() { return m_Settings.Canvas; }
const CanvasSettings& Scene::GetCanvasSettings() const { return m_Settings.Canvas; }

} // namespace CHEngine
