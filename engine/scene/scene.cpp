#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
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
// Scene implementation
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

Scene::~Scene()
{
    // Clean up active signals
    m_Registry.clear();
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
    else if (type == "InputText")
    {
        entity.AddComponent<InputTextControl>();
    }
    else if (type == "ComboBox")
    {
        entity.AddComponent<ComboBoxControl>();
    }
    else if (type == "ProgressBar")
    {
        entity.AddComponent<ProgressBarControl>();
    }
    else if (type == "Image")
    {
        entity.AddComponent<ImageControl>();
    }
    else if (type == "ImageButton")
    {
        entity.AddComponent<ImageButtonControl>();
    }
    else if (type == "Separator")
    {
        entity.AddComponent<SeparatorControl>();
    }
    else if (type == "RadioButton")
    {
        entity.AddComponent<RadioButtonControl>();
    }
    else if (type == "ColorPicker")
    {
        entity.AddComponent<ColorPickerControl>();
    }
    else if (type == "DragFloat")
    {
        entity.AddComponent<DragFloatControl>();
    }
    else if (type == "DragInt")
    {
        entity.AddComponent<DragIntControl>();
    }
    else if (type == "TreeNode")
    {
        entity.AddComponent<TreeNodeControl>();
    }
    else if (type == "TabBar")
    {
        entity.AddComponent<TabBarControl>();
    }
    else if (type == "TabItem")
    {
        entity.AddComponent<TabItemControl>();
    }
    else if (type == "CollapsingHeader")
    {
        entity.AddComponent<CollapsingHeaderControl>();
    }
    else if (type == "PlotLines")
    {
        entity.AddComponent<PlotLinesControl>();
    }
    else if (type == "PlotHistogram")
    {
        entity.AddComponent<PlotHistogramControl>();
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

void Scene::OnUpdateRuntime(Timestep ts)
{
    float deltaTime = ts;
    CH_PROFILE_FUNCTION();

    bool isSim = IsSimulationRunning();
    m_Physics->Update(deltaTime, isSim);

    SceneScripting::Update(this, deltaTime);
    Audio::Update(this, deltaTime);

    // Declarative Scene Transitions (Reactive Logic)
    m_Registry.view<SceneTransitionComponent>().each([&](auto entity, auto& tr) {
        if (tr.Triggered && !tr.TargetScenePath.empty())
        {
            SceneChangeRequestEvent e(tr.TargetScenePath);
            Application::Get().OnEvent(e);
        }
    });
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
    
    if (!audio.SoundPath.empty())
    {
        auto project = Project::GetActive();
        if (project && project->GetAssetManager())
        {
            audio.Asset = project->GetAssetManager()->Get<SoundAsset>(audio.SoundPath);
        }
    }
    
    // Reactive Sound Playback
    if (audio.PlayOnStart && !audio.IsPlaying) {
        audio.IsPlaying = true;
        if (audio.Asset) Audio::Play(audio.Asset, audio.Volume, audio.Pitch, audio.Loop);
    }
}

void Scene::OnModelComponentAdded(entt::registry &reg, entt::entity entity)
{
    auto &component = reg.get<ModelComponent>(entity);
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

void Scene::OnAnimationComponentAdded(entt::registry &reg, entt::entity entity)
{
    auto &component = reg.get<AnimationComponent>(entity);
    // Animation component might need the model asset to perform calculations
    if (reg.all_of<ModelComponent>(entity))
    {
        auto &mc = reg.get<ModelComponent>(entity);
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

Camera3D Scene::GetActiveCamera()
{
    auto view = m_Registry.view<CameraComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto [camera, transform] = view.get<CameraComponent, TransformComponent>(entity);
        if (camera.IsPrimary)
        {
            Camera3D cam3d = { 0 };
            cam3d.position = transform.Translation;
            
            // Convert Euler angles to target vector
            float yaw = transform.Rotation.y;
            float pitch = transform.Rotation.x;
            
            cam3d.target = {
                transform.Translation.x - sinf(yaw) * cosf(pitch),
                transform.Translation.y + sinf(pitch),
                transform.Translation.z - cosf(yaw) * cosf(pitch)
            };
            
            cam3d.up = { 0, 1, 0 };
            cam3d.fovy = camera.Fov;
            cam3d.projection = camera.Projection == 0 ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
            return cam3d;
        }
    }

    // Default fallback camera
    Camera3D fallback = { 0 };
    fallback.position = { 0, 10, 10 };
    fallback.target = { 0, 0, 0 };
    fallback.up = { 0, 1, 0 };
    fallback.fovy = 60.0f;
    fallback.projection = CAMERA_PERSPECTIVE;
    return fallback;
}

} // namespace CHEngine
