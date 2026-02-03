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
        {
            phc.Children.erase(it);
        }
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

    bool isSim = IsSimulationRunning();
    Physics::Update(this, deltaTime, isSim);

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
                bool frameChanged = false;
                while (anim.FrameTimeCounter >= frameTime)
                {
                    anim.CurrentFrame++;
                    anim.FrameTimeCounter -= frameTime;
                    frameChanged = true;

                    if (anim.CurrentFrame >= animations[anim.CurrentAnimationIndex].frameCount)
                    {
                        if (anim.IsLooping)
                        {
                            anim.CurrentFrame = 0;
                        }
                        else
                        {
                            anim.CurrentFrame = animations[anim.CurrentAnimationIndex].frameCount - 1;
                            anim.IsPlaying = false;
                            anim.FrameTimeCounter = 0; // Reset counter if stopped
                            break;
                        }
                    }
                }

                if (frameChanged)
                {
                    // Frame updated, DrawCommand will handle the actual posing during render
                }
            }
            else if (anim.IsPlaying)
            {
                // Animation component exists but no skeleton data
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

    Visuals::DrawScene(this, camera, debugFlags);
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
    ImVec2 referenceSize = refSize;
    if (referenceSize.x <= 0 || referenceSize.y <= 0)
        referenceSize = ImGui::GetIO().DisplaySize;

    // --- UI Layout System ---
    // Pure pixel values and anchors, no custom ReferenceResolution scaling for now
    auto uiView = m_Registry.view<ControlComponent>();
    
    // Process UI elements by ZOrder
    std::vector<entt::entity> sortedList;
    for (auto entityID : uiView) sortedList.push_back(entityID);

    // Simple bubble sort to avoid lambdas
    for (size_t i = 0; i < sortedList.size(); i++) {
        for (size_t j = i + 1; j < sortedList.size(); j++) {
            if (uiView.get<ControlComponent>(sortedList[i]).ZOrder > uiView.get<ControlComponent>(sortedList[j]).ZOrder) {
                std::swap(sortedList[i], sortedList[j]);
            }
        }
    }

    for (entt::entity entityID : sortedList)
    {
        Entity entity{entityID, this};
        auto &cc = uiView.get<ControlComponent>(entityID);
        if (!cc.IsActive) continue;

        if (entity.HasComponent<ButtonControl>())
            entity.GetComponent<ButtonControl>().PressedThisFrame = false;

        // --- Uniform UI Math using member function ---
        auto rect = cc.Transform.CalculateRect(
            {referenceSize.x, referenceSize.y},
            {refPos.x, refPos.y});
        
        ImVec2 pos = {rect.Min.x - refPos.x, rect.Min.y - refPos.y};
        ImVec2 size = {rect.Size().x, rect.Size().y};

        ImGui::SetCursorPos(pos);
        ImGui::BeginGroup();
        ImGui::PushID((int)entityID); // Use stable entity ID
        
        // Render Panel
        if (entity.HasComponent<PanelControl>())
        {
            auto& pnl = entity.GetComponent<PanelControl>();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{pnl.Style.BackgroundColor.r / 255.0f, pnl.Style.BackgroundColor.g / 255.0f, pnl.Style.BackgroundColor.b / 255.0f, pnl.Style.BackgroundColor.a / 255.0f});
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, pnl.Style.Rounding);
            ImGui::BeginChild("Panel", size, true); 
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // Render Label
        if (entity.HasComponent<LabelControl>())
        {
            auto& lbl = entity.GetComponent<LabelControl>();
            ImVec4 color = {lbl.Style.TextColor.r / 255.0f, lbl.Style.TextColor.g / 255.0f, lbl.Style.TextColor.b / 255.0f, lbl.Style.TextColor.a / 255.0f};
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::PushTextWrapPos(pos.x + size.x);

            ImVec2 textSize = ImGui::CalcTextSize(lbl.Text.c_str(), nullptr, true, size.x);
            float startX = pos.x;
            if (lbl.Style.HorizontalAlignment == TextAlignment::Center) 
            {
                startX += (size.x - textSize.x) * 0.5f;
            }
            else if (lbl.Style.HorizontalAlignment == TextAlignment::Right) 
            {
                startX += (size.x - textSize.x);
            }

            float startY = pos.y;
            if (lbl.Style.VerticalAlignment == TextAlignment::Center) 
            {
                startY += (size.y - textSize.y) * 0.5f;
            }
            else if (lbl.Style.VerticalAlignment == TextAlignment::Right) 
            {
                startY += (size.y - textSize.y);
            }

            ImGui::SetCursorPos({startX, startY});
            ImGui::TextUnformatted(lbl.Text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        // Render Button
        if (entity.HasComponent<ButtonControl>())
        {
            auto& btn = entity.GetComponent<ButtonControl>();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{btn.Style.BackgroundColor.r / 255.0f, btn.Style.BackgroundColor.g / 255.0f, btn.Style.BackgroundColor.b / 255.0f, btn.Style.BackgroundColor.a / 255.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{btn.Style.HoverColor.r / 255.0f, btn.Style.HoverColor.g / 255.0f, btn.Style.HoverColor.b / 255.0f, btn.Style.HoverColor.a / 255.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{btn.Style.PressedColor.r / 255.0f, btn.Style.PressedColor.g / 255.0f, btn.Style.PressedColor.b / 255.0f, btn.Style.PressedColor.a / 255.0f});
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{btn.Text.TextColor.r / 255.0f, btn.Text.TextColor.g / 255.0f, btn.Text.TextColor.b / 255.0f, btn.Text.TextColor.a / 255.0f});
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btn.Style.Rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{
                btn.Text.HorizontalAlignment == TextAlignment::Left ? 0.0f : (btn.Text.HorizontalAlignment == TextAlignment::Center ? 0.5f : 1.0f),
                btn.Text.VerticalAlignment == TextAlignment::Left ? 0.0f : (btn.Text.VerticalAlignment == TextAlignment::Center ? 0.5f : 1.0f)
            });
            
            if (ImGui::Button(btn.Label.c_str(), size)) btn.PressedThisFrame = true;

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
        }

        if (entity.HasComponent<SliderControl>())
        {
            auto& sl = entity.GetComponent<SliderControl>();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{sl.Text.TextColor.r / 255.0f, sl.Text.TextColor.g / 255.0f, sl.Text.TextColor.b / 255.0f, sl.Text.TextColor.a / 255.0f});
            ImGui::SetNextItemWidth(size.x * 0.7f); // Make slider 70% of width to leave space for label
            sl.Changed = ImGui::SliderFloat(sl.Label.c_str(), &sl.Value, sl.Min, sl.Max);
            ImGui::PopStyleColor();
        }

        if (entity.HasComponent<CheckboxControl>())
        {
            auto& cb = entity.GetComponent<CheckboxControl>();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{cb.Text.TextColor.r / 255.0f, cb.Text.TextColor.g / 255.0f, cb.Text.TextColor.b / 255.0f, cb.Text.TextColor.a / 255.0f});
            cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
        ImGui::EndGroup();

        // Editor selection and drag support
        if (editMode) {
            ImGui::SetCursorPos(pos);
            ImGui::InvisibleButton("##SelectionZone", size);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                cc.Transform.OffsetMin.x += delta.x;
                cc.Transform.OffsetMax.x += delta.x;
                cc.Transform.OffsetMin.y += delta.y;
                cc.Transform.OffsetMax.y += delta.y;
                m_Registry.patch<ControlComponent>(entityID);
            }
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
