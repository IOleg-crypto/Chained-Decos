#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/scene_scripting.h"
#include "project.h"
#include "raylib.h"
#include "raymath.h"
#include "scene_serializer.h"
#include "scriptable_entity.h"
#include "engine/scene/scene_events.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/script_registry.h"

namespace CHEngine
{
// Scene implementation
Scene::Scene()
{
    m_Registry.ctx().emplace<Scene*>(this);

    // Declarative signals binding
    m_Registry.on_construct<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);
    m_Registry.on_update<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);

    m_Registry.on_construct<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);
    m_Registry.on_update<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);

    m_Registry.on_construct<ColliderComponent>().connect<&Scene::OnColliderComponentAdded>(this);
    m_Registry.on_update<ColliderComponent>().connect<&Scene::OnColliderComponentAdded>(this);

    m_Registry.on_construct<PanelControl>().connect<&Scene::OnPanelControlAdded>(this);
    m_Registry.on_update<PanelControl>().connect<&Scene::OnPanelControlAdded>(this);

    // Every scene must have its own environment to avoid skybox leaking/bugs
    m_Settings.Environment = std::make_shared<EnvironmentAsset>();
    
    // UUID Mapping
    m_Registry.on_construct<IDComponent>().connect<&Scene::OnIDConstruct>(this);
    m_Registry.on_destroy<IDComponent>().connect<&Scene::OnIDDestroy>(this);

    // Hierarchy Mapping
    m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyDestroy>(this);

    // Create physics instance
    m_Physics = std::make_unique<Physics>(this);

    // Create script registry
    m_ScriptRegistry = std::make_unique<ScriptRegistry>();
}

Scene::~Scene()
{
    // Clean up active signals
    m_Registry.clear();
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_INFO("Scene::Copy - Starting copy of scene '{}'", other->m_Settings.Name);

    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // 1. Copy Scene Settings
    newScene->m_Settings = other->m_Settings;
    
    // 2. Copy Script Registry
    if (other->m_ScriptRegistry)
    {
        newScene->m_ScriptRegistry->CopyFrom(*other->m_ScriptRegistry);
        CH_CORE_INFO("Scene::Copy - Script registry copied successfully.");
    }
    
    // 2. Copy Entities (Direct Memory Copy)
    auto &srcRegistry = other->m_Registry;
    auto &dstRegistry = newScene->m_Registry;

    // Copy all entities using ComponentSerializer
    int entityCount = 0;
    srcRegistry.view<IDComponent>().each(
        [&](auto entityHandle, auto& id)
        {
            entityCount++;
            Entity srcEntity = {entityHandle, &other->m_Registry};
            Entity dstEntity = newScene->CreateEntityWithUUID(id.ID);

            ComponentSerializer::CopyAll(srcEntity, dstEntity);

            if (srcEntity.HasComponent<NativeScriptComponent>())
            {
                auto& nsc = srcEntity.GetComponent<NativeScriptComponent>();
                CH_CORE_INFO("  - Entity '{}': Copying NativeScriptComponent with {} scripts", 
                            srcEntity.GetComponent<TagComponent>().Tag, nsc.Scripts.size());
            }
        });

    CH_CORE_INFO("Scene::Copy - Successfully copied {} entities", entityCount);
    return newScene;
}

Entity Scene::CreateEntity(const std::string &name)
{
    Entity entity(m_Registry.create(), &m_Registry);
    entity.AddComponent<IDComponent>();
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created: {} ({})", name, (uint64_t)entity.GetComponent<IDComponent>().ID);
    return entity;
}

Entity Scene::CopyEntity(entt::entity copyEntity)
{
    Entity srcEntity(copyEntity, &m_Registry);
    std::string name = srcEntity.GetName();
    Entity dstEntity = CreateEntity(name);
    
    // Copy components
    ComponentSerializer::CopyAll(srcEntity, dstEntity);
    
    return dstEntity;
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
{
    Entity entity(m_Registry.create(), &m_Registry);
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
    
    if (type == "Button")           entity.AddComponent<ButtonControl>();
    else if (type == "Panel")       entity.AddComponent<PanelControl>();
    else if (type == "Label")       entity.AddComponent<LabelControl>();
    else if (type == "Slider")      entity.AddComponent<SliderControl>();
    else if (type == "CheckBox")    entity.AddComponent<CheckboxControl>();
    else if (type == "InputText")   entity.AddComponent<InputTextControl>();
    else if (type == "ComboBox")    entity.AddComponent<ComboBoxControl>();
    else if (type == "ProgressBar") entity.AddComponent<ProgressBarControl>();
    else if (type == "Image")       entity.AddComponent<ImageControl>();
    else if (type == "ImageButton") entity.AddComponent<ImageButtonControl>();
    else if (type == "Separator")   entity.AddComponent<SeparatorControl>();
    else if (type == "RadioButton") entity.AddComponent<RadioButtonControl>();
    else if (type == "ColorPicker") entity.AddComponent<ColorPickerControl>();
    else if (type == "DragFloat")   entity.AddComponent<DragFloatControl>();
    else if (type == "DragInt")     entity.AddComponent<DragIntControl>();
    else if (type == "TreeNode")    entity.AddComponent<TreeNodeControl>();
    else if (type == "TabBar")      entity.AddComponent<TabBarControl>();
    else if (type == "TabItem")     entity.AddComponent<TabItemControl>();
    else if (type == "CollapsingHeader") entity.AddComponent<CollapsingHeaderControl>();
    else if (type == "PlotLines")   entity.AddComponent<PlotLinesControl>();
    else if (type == "PlotHistogram") entity.AddComponent<PlotHistogramControl>();

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
            DestroyEntity(Entity(childHandle, &m_Registry));
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

void Scene::OnUpdateRuntime(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateScripting(timestep);
    UpdateAnimations(timestep);
    UpdateAudio(timestep);
    UpdateCameras(timestep);
    UpdateTransitions();
    UpdatePhysics(timestep);
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateAnimations(timestep);
    UpdateCameras(timestep);
    UpdatePhysics(timestep);
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
    auto view = m_Registry.view<CameraComponent>();
    for (auto entity : view)
    {
        auto& cameraComponent = view.get<CameraComponent>(entity);
        if (!cameraComponent.FixedAspectRatio)
            cameraComponent.Camera.SetViewportSize(width, height);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Update Logic Implementation
// -------------------------------------------------------------------------------------------------------------------

void Scene::UpdatePhysics(Timestep deltaTime)
{
    m_Physics->Update(deltaTime, IsSimulationRunning());
}

void Scene::UpdateAnimations(Timestep deltaTime)
{
    m_Registry.view<ModelComponent, AnimationComponent>().each([&](auto entity, auto& mc, auto& anim) {
        if (anim.IsPlaying && mc.Asset)
        {
            const auto& anims = mc.Asset->GetRawAnimations();
            int animCount = (int)anims.size();
            if (anim.CurrentAnimationIndex >= 0 && anim.CurrentAnimationIndex < animCount)
            {
                const auto& rawAnim = anims[anim.CurrentAnimationIndex];
                float targetFPS = 30.0f;
                if (Project::GetActive())
                    targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
                float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);
                
                anim.FrameTimeCounter += deltaTime;
                while (anim.FrameTimeCounter >= frameTime)
                {
                    anim.CurrentFrame++;
                    anim.FrameTimeCounter -= frameTime;
                    if (anim.CurrentFrame >= rawAnim.frameCount)
                    {
                        if (anim.IsLooping) anim.CurrentFrame = 0;
                        else {
                            anim.CurrentFrame = rawAnim.frameCount - 1;
                            anim.IsPlaying = false;
                        }
                    }
                }
            }
        }
    });
}

void Scene::UpdateScripting(Timestep deltaTime)
{
    SceneScripting::Update(this, deltaTime);
}

void Scene::UpdateAudio(Timestep deltaTime)
{
    Audio::Update(this, deltaTime);
}

void Scene::UpdateTransitions()
{
    m_Registry.view<SceneTransitionComponent>().each([&](auto entity, auto& tr) {
        if (tr.Triggered && !tr.TargetScenePath.empty())
        {
            SceneChangeRequestEvent e(tr.TargetScenePath);
            Application::Get().OnEvent(e);
        }
    });
}

void Scene::UpdateCameras(Timestep deltaTime)
{
    m_Registry.view<TransformComponent, CameraComponent>().each([&](auto entity, auto& tc, auto& cc) {
        if (cc.IsOrbitCamera && !cc.TargetEntityTag.empty())
        {
            Entity target = FindEntityByTag(cc.TargetEntityTag);
            if (!target) return;

            auto& targetTc = target.GetComponent<TransformComponent>();
            
            // Spherical coordinates → orbit position
            float yaw = cc.OrbitYaw * DEG2RAD;
            float pitch = cc.OrbitPitch * DEG2RAD;
            Vector3 offset = {
                cc.OrbitDistance * sin(yaw) * cos(pitch),
                cc.OrbitDistance * sin(pitch),
                cc.OrbitDistance * cos(yaw) * cos(pitch)
            };
            tc.Translation = Vector3Add(targetTc.Translation, offset);
            
            // LookAt → quaternion rotation
            Matrix viewMat = MatrixLookAt(tc.Translation, targetTc.Translation, {0, 1, 0});
            tc.RotationQuat = QuaternionFromMatrix(MatrixInvert(viewMat));
            
            // Sync Euler angles for Inspector display
            tc.Rotation = QuaternionToEuler(tc.RotationQuat);
            tc.Rotation.x *= RAD2DEG;
            tc.Rotation.y *= RAD2DEG;
            tc.Rotation.z *= RAD2DEG;
        }
    });
}


// -------------------------------------------------------------------------------------------------------------------

void Scene::OnRuntimeStart()
{
    m_IsSimulationRunning = true;
    
    std::string sceneName = "Unknown";
    auto view = m_Registry.view<TagComponent>();
    if (!view.empty())
        sceneName = view.get<TagComponent>(view.front()).Tag;

    
    // Track initialization statistics
    int modelsLoaded = 0, modelsFailed = 0;
    int collidersBuilt = 0, collidersFailed = 0;
    int scriptsCreated = 0;
    
    // Phase 1: Load models (other components may depend on them)
    m_Registry.view<ModelComponent>().each([&](auto entity, auto& component) {
        if (!component.ModelPath.empty()) {
            OnModelComponentAdded(m_Registry, entity);
            if (component.Asset && component.Asset->IsReady())
                modelsLoaded++;
            else {
                modelsFailed++;
                CH_CORE_ERROR("Failed to load model: {}", component.ModelPath);
            }
        }
    });
    
    // Phase 2: Initialize ColliderComponents (may depend on models for mesh colliders)
    m_Registry.view<ColliderComponent>().each([&](auto entity, auto& component) {
        OnColliderComponentAdded(m_Registry, entity);
        if (component.Type == ColliderType::Mesh) {
            if (component.BVHRoot) {
                collidersBuilt++;
            } else if (!component.ModelPath.empty()) {
                collidersFailed++;
                CH_CORE_ERROR("Failed to build BVH for collider: {}", component.ModelPath);
            }
        }
    });

    // Phase 3: Initialize NativeScripts
    m_Registry.view<NativeScriptComponent>().each([&](auto entity, auto& nsc) {
        std::string entityTag = m_Registry.any_of<TagComponent>(entity) ? m_Registry.get<TagComponent>(entity).Tag : "Unnamed";
        for (auto& script : nsc.Scripts) {
            if (script.InstantiateScript) {
                script.Instance = script.InstantiateScript();
                if (script.Instance) {
                    script.Instance->m_Entity = Entity(entity, &m_Registry);
                    script.Instance->m_Scene = this;
                    CH_CORE_INFO("[SCRIPT_DIAG] Scene: '{}' - Script '{}' instantiated for entity '{}'", 
                                sceneName, script.ScriptName, entityTag);
                    script.Instance->OnCreate();
                    scriptsCreated++;
                } else {
                    CH_CORE_ERROR("[SCRIPT_DIAG] Scene: '{}' - Failed to instantiate script: {} for entity '{}'", 
                                sceneName, script.ScriptName, entityTag);
                }
            } else {
                CH_CORE_WARN("[SCRIPT_DIAG] Scene: '{}' - Entity '{}' has script '{}' but no InstantiateScript func!",
                            sceneName, entityTag, script.ScriptName);
            }
        }
    });

    // Phase 4: Initialize UI textures (PanelControl)
    m_Registry.view<PanelControl>().each([&](auto entity, auto& panel) {
        OnPanelControlAdded(m_Registry, entity);
    });
    
    // Report initialization results
    CH_CORE_INFO("Runtime initialization complete:");
    CH_CORE_INFO("  - Models: {} loaded, {} failed", modelsLoaded, modelsFailed);
    CH_CORE_INFO("  - Colliders: {} built, {} failed", collidersBuilt, collidersFailed);
    CH_CORE_INFO("  - Scripts: {} created", scriptsCreated);
}

void Scene::OnRuntimeStop()
{
    CH_CORE_INFO("Scene - Stopping runtime simulation");
    
    // Destroy all NativeScript instances
    m_Registry.view<NativeScriptComponent>().each([](auto entity, auto& nsc) {
        for (auto& script : nsc.Scripts) {
            if (script.Instance) {
                script.Instance->OnDestroy();
                script.DestroyScript(&script);
                script.Instance = nullptr;
            }
        }
    });
    
    m_IsSimulationRunning = false;
    CH_CORE_INFO("Runtime stopped - all scripts destroyed");
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
            return {entity, &m_Registry};
    }
    return {};
}
Entity Scene::GetEntityByUUID(UUID uuid)
{
    if (m_EntityMap.find(uuid) != m_EntityMap.end())
        return {m_EntityMap.at(uuid), &m_Registry};
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
    if (component.ModelPath.empty())
        return;

    auto project = Project::GetActive();
    std::string resolvedPath = component.ModelPath;
    if (project && project->GetAssetManager())
        resolvedPath = project->GetAssetManager()->ResolvePath(component.ModelPath);
    
    bool pathChanged = !component.Asset || (component.Asset->GetPath() != resolvedPath);

    if (pathChanged)
    {
        if (project && project->GetAssetManager())
        {
            component.Asset = project->GetAssetManager()->Get<ModelAsset>(component.ModelPath);
            if (!component.Asset)
                CH_CORE_ERROR("Failed to load model asset: {}", component.ModelPath);
        }
        else
        {
            CH_CORE_ERROR("No active project or AssetManager!");
        }
        
        // Only reset materials if we don't have deserialized ones
        if (component.Materials.empty())
            component.MaterialsInitialized = false;
    }

    if (component.Asset && !component.MaterialsInitialized)
    {
        if (component.Asset->GetState() != AssetState::Ready)
            return;
        
        Model &model = component.Asset->GetModel();
        
        // Initialize default materials from model if none were deserialized
        if (component.Materials.empty() && model.materials != nullptr)
        {
            std::set<int> uniqueIndices;
            for (int i = 0; i < model.meshCount; i++)
                uniqueIndices.insert(model.meshMaterial[i]);

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


void Scene::OnColliderComponentAdded(entt::registry &reg, entt::entity entity)
{
    auto &collider = reg.get<ColliderComponent>(entity);
    if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
    {
        // Rebuild BVH if path changed or it's null
        if (!collider.BVHRoot)
        {
            auto project = Project::GetActive();
            if (project && project->GetAssetManager())
            {
                auto modelAsset = project->GetAssetManager()->Get<ModelAsset>(collider.ModelPath);
                if (modelAsset && modelAsset->IsReady())
                {
                    collider.BVHRoot = BVH::Build(modelAsset);
                }
            }
        }
    }
}

void Scene::OnPanelControlAdded(entt::registry &reg, entt::entity entity)
{
    auto &panel = reg.get<PanelControl>(entity);
    if (panel.TexturePath.empty())
    {
        return;
    }
    
    // Load texture asset if path changed or texture is null
    bool pathChanged = !panel.Texture || (panel.Texture->GetPath() != panel.TexturePath);
    
    if (pathChanged)
    {
        auto project = Project::GetActive();
        if (project && project->GetAssetManager())
        {
            panel.Texture = project->GetAssetManager()->Get<TextureAsset>(panel.TexturePath);
        }
    }
}

std::optional<Camera3D> Scene::GetActiveCamera()
{
    auto view = m_Registry.view<TransformComponent, CameraComponent>();
    
    entt::entity fallbackEntity = entt::null;

    for (auto entityHandle : view)
    {
        auto &cc = view.get<CameraComponent>(entityHandle);
        if (fallbackEntity == entt::null) fallbackEntity = entityHandle;

        if (cc.Primary)
        {
            return GetCameraFromEntity(entityHandle);
        }
    }

    // Fallback to first available camera if no primary set
    if (fallbackEntity != entt::null)
    {
        return GetCameraFromEntity(fallbackEntity);
    }

    return std::nullopt;
}

Camera3D Scene::GetCameraFromEntity(entt::entity entityHandle)
{
    auto &tc = m_Registry.get<TransformComponent>(entityHandle);
    auto &cc = m_Registry.get<CameraComponent>(entityHandle);
    
    Camera3D camera = {0};
    camera.position = tc.Translation;
    
    // Calculate forward vector from rotation
    Matrix frame = QuaternionToMatrix(tc.RotationQuat);
    Vector3 forward = Vector3Transform({ 0, 0, -1 }, frame);
    
    camera.target = Vector3Add(camera.position, forward);
    camera.up = Vector3Transform({ 0, 1, 0 }, frame);
    
    // Map Hazel-style SceneCamera properties to Raylib's Camera3D
    if (cc.Camera.GetProjectionType() == ProjectionType::Perspective)
    {
        camera.fovy = cc.Camera.GetPerspectiveVerticalFOV() * RAD2DEG;
        camera.projection = CAMERA_PERSPECTIVE;
    }
    else
    {
        camera.fovy = cc.Camera.GetOrthographicSize();
        camera.projection = CAMERA_ORTHOGRAPHIC;
    }

    return camera;
}

Entity Scene::GetPrimaryCameraEntity()
{
    auto view = m_Registry.view<CameraComponent>();
    for (auto entity : view)
    {
        const auto& camera = view.get<CameraComponent>(entity);
        if (camera.Primary)
            return Entity(entity, &m_Registry);
    }
    return {};
}

} // namespace CHEngine
