#include "engine/scene/scene.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/visuals.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene_scripting.h"
#include "project.h"
#include "raylib.h"
#include "raymath.h"
#include "scene_serializer.h"
#include "scriptable_entity.h"

namespace CHEngine
{
void Scene::RequestSceneChange(const std::string &path)
{
    Application::Get().RequestSceneChange(path);
}
Scene::Scene()
{
    // Declarative signals binding
    m_Registry.on_construct<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);
    m_Registry.on_update<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);

    m_Registry.on_construct<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);
    m_Registry.on_update<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);

    m_Registry.on_construct<AudioComponent>().connect<&Scene::OnAudioComponentAdded>(this);
    m_Registry.on_update<AudioComponent>().connect<&Scene::OnAudioComponentAdded>(this);

    // UUID Mapping
    m_Registry.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    m_Registry.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    // Hierarchy Mapping
    m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyDestroy>(this);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();
}

Scene::~Scene()
{
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

    using UIFactoryFn = void (*)(Entity);
    static const std::unordered_map<std::string_view, UIFactoryFn> dispatchTable = {
        {"Button", [](Entity e) { e.AddComponent<ButtonControl>(); }},
        {"Panel", [](Entity e) { e.AddComponent<PanelControl>(); }},
        {"Label", [](Entity e) { e.AddComponent<LabelControl>(); }},
        {"Slider", [](Entity e) { e.AddComponent<SliderControl>(); }},
        {"CheckBox", [](Entity e) { e.AddComponent<CheckboxControl>(); }},
    };

    if (auto it = dispatchTable.find(type); it != dispatchTable.end())
        it->second(entity);

    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    CH_CORE_ASSERT(entity, "Entity is null!");
    CH_CORE_INFO("Entity Destroyed: {}", (uint32_t)entity);
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
            phc.Children.erase(it);
    }

    // 2. Clear parent link in children (detaching them, not destroying)
    for (auto child : hc.Children)
    {
        if (reg.valid(child) && reg.all_of<HierarchyComponent>(child))
        {
            reg.get<HierarchyComponent>(child).Parent = entt::null;
        }
    }
}

template <> void Scene::OnComponentAdded<ModelComponent>(Entity entity, ModelComponent &component)
{
    if (!component.ModelPath.empty())
    {
        // Check if we need to (re)load the asset
        bool pathChanged = !component.Asset || (component.Asset->GetPath() != component.ModelPath);

        if (pathChanged)
        {
            // component.Asset = AssetManager::Get<ModelAsset>(component.ModelPath);
            component.Asset = nullptr;
            component.MaterialsInitialized = false; // Reset materials if asset changed
        }

        if (component.Asset && !component.MaterialsInitialized)
        {
            Model &model = component.Asset->GetModel();
            component.Materials.clear();
            std::set<int> uniqueIndices;
            for (int i = 0; i < model.meshCount; i++)
                uniqueIndices.insert(model.meshMaterial[i]);

            for (int idx : uniqueIndices)
            {
                MaterialSlot slot("Material " + std::to_string(idx), idx);
                slot.Target = MaterialSlotTarget::MaterialIndex;
                slot.Material.AlbedoColor = model.materials[idx].maps[MATERIAL_MAP_ALBEDO].color;
                component.Materials.push_back(slot);
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
            // mc.Asset = AssetManager::Get<ModelAsset>(mc.ModelPath);
            mc.Asset = nullptr;
        }
    }
}

template <> void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent &component)
{
    if (!component.SoundPath.empty())
    {
        // component.Asset = AssetManager::Get<SoundAsset>(component.SoundPath);
        component.Asset = nullptr;
    }
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    CH_PROFILE_FUNCTION();

    // Declarative Pipeline
    SceneScripting::Update(this, deltaTime);
    UpdateAnimation(deltaTime);
    UpdateAudio(deltaTime);

    // Declarative Scene Transitions
    auto view = m_Registry.view<SceneTransitionComponent>();
    for (auto entity : view)
    {
        auto &transition = view.get<SceneTransitionComponent>(entity);
        if (transition.Triggered && !transition.TargetScenePath.empty())
        {
            RequestSceneChange(transition.TargetScenePath);
            break; // Only one transition at a time
        }
    }
}

void Scene::UpdateScripting(float deltaTime)
{
    // High-level scripting logic now handled by SceneScripting
}

void Scene::UpdateAnimation(float deltaTime)
{
    auto animView = m_Registry.view<AnimationComponent, ModelComponent>();
    for (auto entity : animView)
    {
        auto &anim = animView.get<AnimationComponent>(entity);
        auto &model = animView.get<ModelComponent>(entity);

        if (anim.IsPlaying && model.Asset)
        {
            int animCount = 0;
            auto *animations = model.Asset->GetAnimations(&animCount);

            if (animations && anim.CurrentAnimationIndex < animCount)
            {
                anim.FrameTimeCounter += deltaTime;
                float targetFPS = 30.0f;
                if (Project::GetActive())
                    targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;

                float frameTime = 1.0f / targetFPS;
                if (anim.FrameTimeCounter >= frameTime)
                {
                    anim.CurrentFrame++;
                    anim.FrameTimeCounter = 0;
                    if (anim.CurrentFrame >= animations[anim.CurrentAnimationIndex].frameCount)
                    {
                        if (anim.IsLooping)
                            anim.CurrentFrame = 0;
                        else
                        {
                            anim.CurrentFrame =
                                animations[anim.CurrentAnimationIndex].frameCount - 1;
                            anim.IsPlaying = false;
                        }
                    }
                    model.Asset->UpdateAnimation(anim.CurrentAnimationIndex, anim.CurrentFrame);
                }
            }
        }
    }
}

void Scene::UpdateAudio(float deltaTime)
{
    m_Registry.view<AudioComponent>().each(
        [](auto entity, auto &audio)
        {
            if (audio.Asset && audio.PlayOnStart)
            {
                Sound &sound = audio.Asset->GetSound();
                SetSoundVolume(sound, audio.Volume);
                SetSoundPitch(sound, audio.Pitch);
                ::PlaySound(sound);
                audio.PlayOnStart = false;
            }
        });
}

void Scene::OnRender(const Camera3D &camera, const DebugRenderFlags *debugFlags)
{
    CH_PROFILE_FUNCTION();
    Visuals::DrawScene(this, camera, debugFlags);
}

Camera3D Scene::GetActiveCamera() const
{
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

    EnvironmentSettings settings;
    settings.Skybox.TexturePath = m_Settings.Skybox.TexturePath;
    settings.Skybox.Exposure = m_Settings.Skybox.Exposure;
    settings.Skybox.Brightness = m_Settings.Skybox.Brightness;
    settings.Skybox.Contrast = m_Settings.Skybox.Contrast;
    return settings;
}

// -------------------------------------------------------------------------------------------------------------------

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}

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
    ImVec2 referenceSize = refSize;
    if (referenceSize.x <= 0 || referenceSize.y <= 0)
        referenceSize = ImGui::GetIO().DisplaySize;

    // Iterate through only ROOT entities with ControlComponent
    m_Registry.view<ControlComponent>().each(
        [&](auto entityID, auto &cc)
        {
            Entity entity{entityID, this};

            // Check if this is a root control
            bool isRoot = true;
            if (entity.HasComponent<HierarchyComponent>())
            {
                auto parent = entity.GetComponent<HierarchyComponent>().Parent;
                if (parent != entt::null && m_Registry.all_of<ControlComponent>(parent))
                    isRoot = false;
            }

            if (isRoot)
            {
                // TODO: Implement new Control rendering
                // CanvasRenderer::DrawEntity(entity, refPos, referenceSize, m_CanvasSettings,
                // editMode);
            }
        });

    // Scripted UI elements
    SceneScripting::RenderUI(this);
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
    OnComponentAdded<AudioComponent>(Entity{entity, this}, reg.get<AudioComponent>(entity));
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
} // namespace CHEngine
