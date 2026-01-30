#include "engine/scene/scene.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
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

    if (type == "Button") entity.AddComponent<ButtonControl>();
    else if (type == "Panel") entity.AddComponent<PanelControl>();
    else if (type == "Label") entity.AddComponent<LabelControl>();
    else if (type == "Slider") entity.AddComponent<SliderControl>();
    else if (type == "CheckBox") entity.AddComponent<CheckboxControl>();

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
            component.Asset = AssetManager::Get<ModelAsset>(component.ModelPath);
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
            mc.Asset = AssetManager::Get<ModelAsset>(mc.ModelPath);
        }
    }
}

template <> void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent &component)
{
    if (!component.SoundPath.empty())
    {
        component.Asset = AssetManager::Get<SoundAsset>(component.SoundPath);
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

const struct SkyboxComponent &Scene::GetSkybox() const
{
    if (m_Settings.Environment)
    {
        // Proxy from environment
        const auto &envSky = m_Settings.Environment->GetSettings().Skybox;
        const_cast<Scene *>(this)->m_ProxySkybox.TexturePath = envSky.TexturePath;
        const_cast<Scene *>(this)->m_ProxySkybox.Exposure = envSky.Exposure;
        const_cast<Scene *>(this)->m_ProxySkybox.Brightness = envSky.Brightness;
        const_cast<Scene *>(this)->m_ProxySkybox.Contrast = envSky.Contrast;
        return m_ProxySkybox;
    }
    return m_Settings.Skybox;
}

struct SkyboxComponent &Scene::GetSkybox()
{
    return m_Settings.Skybox;
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
    // Assets are now managed declaratively via entt signals
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

    // --- Declarative GUISystem ---
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (m_Settings.Canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize)
    {
        scaleX = referenceSize.x / m_Settings.Canvas.ReferenceResolution.x;
        scaleY = referenceSize.y / m_Settings.Canvas.ReferenceResolution.y;

        float scale = scaleX * (1.0f - m_Settings.Canvas.MatchWidthOrHeight) +
                      scaleY * m_Settings.Canvas.MatchWidthOrHeight;
        scaleX = scale;
        scaleY = scale;
    }

    auto view = m_Registry.view<ControlComponent>();
    
    // Sort by ZOrder
    std::vector<entt::entity> sortedEntities(view.begin(), view.end());
    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
    });

    for (auto entityID : sortedEntities)
    {
        Entity entity{entityID, this};
        auto &cc = view.get<ControlComponent>(entityID);
        if (!cc.IsActive) continue;

        // Position based on RectTransform and Scaling
        ImVec2 pos;
        pos.x = cc.Transform.AnchorMin.x * refSize.x + cc.Transform.OffsetMin.x * scaleX;
        pos.y = cc.Transform.AnchorMin.y * refSize.y + cc.Transform.OffsetMin.y * scaleY;
        
        ImVec2 size;
        size.x = (cc.Transform.AnchorMax.x * refSize.x + cc.Transform.OffsetMax.x * scaleX) - pos.x;
        size.y = (cc.Transform.AnchorMax.y * refSize.y + cc.Transform.OffsetMax.y * scaleY) - pos.y;

        ImGui::SetCursorPos(pos);
        ImGui::BeginGroup();
        ImGui::PushID((int)entityID);
        
        // Declarative UI Rendering
        auto draw = [&](auto type, auto&& func) {
            using T = std::decay_t<decltype(type)>;
            if (entity.HasComponent<T>()) func(entity.GetComponent<T>());
        };

        draw(PanelControl{}, [&](auto &pnl) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{pnl.Style.BackgroundColor.r / 255.0f, pnl.Style.BackgroundColor.g / 255.0f, pnl.Style.BackgroundColor.b / 255.0f, pnl.Style.BackgroundColor.a / 255.0f});
            ImGui::BeginChild("Panel", size, true); 
            ImGui::EndChild();
            ImGui::PopStyleColor();
        });

        draw(LabelControl{}, [&](auto &lbl) {
            ImGui::Text("%s", lbl.Text.c_str());
        });

        draw(ButtonControl{}, [&](auto &btn) {
            if (ImGui::Button(btn.Label.c_str(), size)) 
            {
                btn.PressedThisFrame = true;
                
                // Execute built-in actions
                if (btn.Action == ButtonAction::LoadScene && !btn.TargetScene.empty())
                {
                    RequestSceneChange(btn.TargetScene);
                }
                else if (btn.Action == ButtonAction::Quit)
                {
                    Application::Get().Close();
                }
            }
        });

        draw(SliderControl{}, [&](auto &sl) {
            sl.Changed = ImGui::SliderFloat("##Slider", &sl.Value, sl.Min, sl.Max);
        });

        draw(CheckboxControl{}, [&](auto &cb) {
            cb.Changed = ImGui::Checkbox("##Checkbox", &cb.Checked);
        });

        ImGui::PopID();
        ImGui::EndGroup();

        // Drag-to-move logic (Editor only)
        if (editMode && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            cc.Transform.OffsetMin.x += delta.x / scaleX;
            cc.Transform.OffsetMax.x += delta.x / scaleX;
            cc.Transform.OffsetMin.y += delta.y / scaleY;
            cc.Transform.OffsetMax.y += delta.y / scaleY;
            
            // Trigger registry patch
            m_Registry.patch<ControlComponent>(entityID);
        }
    }

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
