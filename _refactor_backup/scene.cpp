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
#include "engine/physics/bvh/bvh.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/script_registry.h"

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
        newScene->m_ScriptRegistry->CopyFrom(*other->m_ScriptRegistry);
    
    // 2. Copy Entities (Direct Memory Copy)
    auto &srcRegistry = other->m_Registry;
    auto &dstRegistry = newScene->m_Registry;

    // Copy all entities using ComponentSerializer
    int entityCount = 0;
    srcRegistry.view<IDComponent>().each(
        [&](auto entityHandle, auto& id)
        {
            entityCount++;
            Entity srcEntity = {entityHandle, other.get()};
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

    static int frameCount = 0;
    bool isSim = IsSimulationRunning();
    if (frameCount % 60 == 0) {
        CH_CORE_INFO("[SCENE_DIAG] OnUpdateRuntime - dt={}, Sim={}", deltaTime, isSim);
    }
    frameCount++;
    
    UpdateScripting(deltaTime);
    UpdateAnimations(deltaTime);
    UpdateAudio(deltaTime);
    UpdateCameras(deltaTime);
    UpdateTransitions();
    UpdatePhysics(deltaTime);
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

void Scene::UpdatePhysics(float deltaTime)
{
    m_Physics->Update(deltaTime, IsSimulationRunning());
}

void Scene::UpdateAnimations(float deltaTime)
{
    m_Registry.view<ModelComponent, AnimationComponent>().each([&](auto entity, auto& mc, auto& anim) {
        if (anim.IsPlaying && mc.Asset)
        {
            int animCount = 0;
            auto* anims = mc.Asset->GetAnimations(&animCount);
            if (anims && anim.CurrentAnimationIndex < animCount)
            {
                float targetFPS = 30.0f;
                if (Project::GetActive())
                    targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
                float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);
                
                anim.FrameTimeCounter += deltaTime;
                while (anim.FrameTimeCounter >= frameTime)
                {
                    anim.CurrentFrame++;
                    anim.FrameTimeCounter -= frameTime;
                    if (anim.CurrentFrame >= anims[anim.CurrentAnimationIndex].frameCount)
                    {
                        if (anim.IsLooping) anim.CurrentFrame = 0;
                        else {
                            anim.CurrentFrame = anims[anim.CurrentAnimationIndex].frameCount - 1;
                            anim.IsPlaying = false;
                        }
                    }
                }
            }
        }
    });
}

void Scene::UpdateScripting(float deltaTime)
{
    SceneScripting::Update(this, deltaTime);
}

void Scene::UpdateAudio(float deltaTime)
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

void Scene::UpdateCameras(float deltaTime)
{
    m_Registry.view<TransformComponent, CameraComponent>().each([&](auto entity, auto& tc, auto& cc) {
        if (cc.IsOrbitCamera && !cc.TargetEntityTag.empty())
        {
            Entity target = FindEntityByTag(cc.TargetEntityTag);
            if (target)
            {
                auto& targetTc = target.GetComponent<TransformComponent>();
                
                // Calculate orbit position
                float dist = cc.OrbitDistance;
                float yaw = cc.OrbitYaw * DEG2RAD;
                float pitch = cc.OrbitPitch * DEG2RAD;

                // Spherical coordinates
                float x = dist * sin(yaw) * cos(pitch);
                float y = dist * sin(pitch);
                float z = dist * cos(yaw) * cos(pitch);

                Vector3 offset = {x, y, z};
                Vector3 newPos = Vector3Add(targetTc.Translation, offset);
                
                // Update transform
                tc.Translation = newPos;
                
                // Look at target
                Vector3 direction = Vector3Normalize(Vector3Subtract(targetTc.Translation, newPos));
                
                // Calculate rotation quaternion (LookAt equivalent)
                Matrix lookAt = MatrixLookAt(newPos, targetTc.Translation, {0, 1, 0});
                // MatrixLookAt returns a view matrix (inverse camera transform). 
                // We need the camera's world transform.
                // Invert it (or just use LookAt logic to get Frame)
                
                // Actually, QuaternionFromVector3ToVector3 or similar might be better.
                // Or simplified: Just set the rotation to face the target.
                // Easiest is to construct a rotation matrix from forward/up/right
                
                // Forward is direction to target
                Vector3 forward = direction;
                Vector3 up = {0, 1, 0};
                Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));
                // Re-calculate up to be orthogonal
                up = Vector3CrossProduct(right, forward);
                
                // Matrix (Column Major for Raylib?)
                // Raylib Matrix is row-major in memory but math assumes column vectors?
                // Actually Raylib is column-major.
                
                // Let's use QuaternionFromMatrix
                Matrix transform = MatrixIdentity();
                transform.m0 = right.x; transform.m4 = up.x; transform.m8 = -forward.x; // Render forward is -Z
                transform.m1 = right.y; transform.m5 = up.y; transform.m9 = -forward.y;
                transform.m2 = right.z; transform.m6 = up.z; transform.m10 = -forward.z;
                
                // Wait, MatrixLookAt gives View Matrix. Inverse of View Matrix is Camera World Matrix.
                // MatrixLookAt:
                // zaxis = normal(eye - target)
                // xaxis = normal(cross(up, zaxis))
                // yaxis = cross(zaxis, xaxis)
                // View = { xaxis, yaxis, zaxis }...
                
                // Direct approach:
                // Forward = Target - Pos
                // If we want Forward to be -Z (OpenGL convention):
                // Z = -(Target - Pos) = Pos - Target
                
                // Let's try simple LookAt logic:
                // We want the camera to look AT the target.
                // We can just calculate the rotation from the position and target.
                
                // Simpler: QuaternionLookAt
                // Assuming we want the camera's local -Z to point directly at the target.
                
                // Let's stick to Euler for now if Quaternion is complex?
                // No, we use RotationQuat.
                
                // Raymath MatrixLookAt is:
                // MatrixLookAt(eye, target, up)
                Matrix viewMat = MatrixLookAt(newPos, targetTc.Translation, {0, 1, 0});
                Matrix worldMat = MatrixInvert(viewMat);
                tc.RotationQuat = QuaternionFromMatrix(worldMat);
                
                // Update Euler for Inspector (optional but good for debugging)
                tc.Rotation = QuaternionToEuler(tc.RotationQuat);
                tc.Rotation.x *= RAD2DEG;
                tc.Rotation.y *= RAD2DEG;
                tc.Rotation.z *= RAD2DEG;
            }
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

    CH_CORE_INFO("Scene '{}' - Starting runtime simulation", sceneName);
    
    // Track initialization statistics
    int modelsLoaded = 0, modelsFailed = 0;
    int collidersBuilt = 0, collidersFailed = 0;
    int scriptsCreated = 0;
    
    // Phase 1: Initialize ModelComponents (must load first - other components may depend on models)
    std::vector<std::shared_ptr<ModelAsset>> modelsToWait;
    m_Registry.view<ModelComponent>().each([&](auto entity, auto& component) {
        if (!component.ModelPath.empty()) {
            OnModelComponentAdded(m_Registry, entity);
            if (component.Asset) {
                modelsToWait.push_back(component.Asset);
            }
        }
    });

    // // Wait for models to be ready (with a timeout of 2 seconds)
    // if (!modelsToWait.empty())
    // {
    //     CH_CORE_INFO("Scene: Waiting for {} models to load...", modelsToWait.size());
    //     auto startWait = std::chrono::steady_clock::now();
    //     bool allReady = false;
    //     while (!allReady)
    //     {
    //         allReady = true;
    //         for (auto& asset : modelsToWait) {
    //             if (!asset->IsReady()) {
    //                 allReady = false;
    //                 break;
    //             }
    //         }

    //         auto now = std::chrono::steady_clock::now();
    //         if (std::chrono::duration_cast<std::chrono::seconds>(now - startWait).count() > 2) {
    //             CH_CORE_WARN("Scene: Timeout waiting for models!");
    //             break;
    //         }
            
    //         // Avoid tight loop
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     }
    // }

    // Re-verify model states and log results
    m_Registry.view<ModelComponent>().each([&](auto entity, auto& component) {
        if (!component.ModelPath.empty()) {
            if (component.Asset && component.Asset->IsReady()) {
                modelsLoaded++;
            } else {
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
        for (auto& script : nsc.Scripts) {
            if (script.InstantiateScript) {
                script.Instance = script.InstantiateScript();
                if (script.Instance) {
                    script.Instance->m_Entity = Entity{entity, this};
                    CH_CORE_INFO("  - Script '{}' instantiated successfully for entity '{}'", 
                                script.ScriptName, m_Registry.get<TagComponent>(entity).Tag);
                    script.Instance->OnCreate();
                    scriptsCreated++;
                } else {
                    CH_CORE_ERROR("Failed to instantiate script: {} for entity '{}'", 
                                script.ScriptName, m_Registry.get<TagComponent>(entity).Tag);
                }
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
    CH_CORE_INFO(">>> Scene::OnModelComponentAdded called for entity {}. Path={}, ExistingAsset={}", 
        (uint32_t)entity, component.ModelPath, (void*)component.Asset.get());
    if (component.ModelPath.empty())
    {
        CH_CORE_TRACE("Scene::OnModelComponentAdded - Empty ModelPath, skipping");
        return;
    }

    // Check if we need to (re)load the asset
    auto project = Project::GetActive();
    std::string resolvedPath = component.ModelPath;
    if (project && project->GetAssetManager())
    {
        resolvedPath = project->GetAssetManager()->ResolvePath(component.ModelPath);
    }
    
    bool pathChanged = !component.Asset || (component.Asset->GetPath() != resolvedPath);

    if (pathChanged)
    {
        if (project && project->GetAssetManager())
        {
            CH_CORE_WARN(">>> LOADING ASSET: {}", component.ModelPath);
            component.Asset = project->GetAssetManager()->Get<ModelAsset>(component.ModelPath);
            
            if (component.Asset)
            {
                CH_CORE_WARN(">>> ASSET STATE: {}", (int)component.Asset->GetState());
            }
            else
            {
                CH_CORE_ERROR("Scene::OnModelComponentAdded - Failed to get asset: {}", component.ModelPath);
            }
        }
        else
        {
            CH_CORE_ERROR("Scene::OnModelComponentAdded - No active project or AssetManager!");
        }
        
        // BUG FIX: Only reset MaterialsInitialized if we don't have deserialized materials
        // This prevents discarding materials that were loaded from the scene file
        if (component.Materials.empty())
        {
            component.MaterialsInitialized = false;
        }
        else
        {
            CH_CORE_WARN(">>> PRESERVING {} MATERIALS", component.Materials.size());
        }
    }

    if (component.Asset && !component.MaterialsInitialized)
    {
        if (component.Asset->GetState() != AssetState::Ready)
        {
            CH_CORE_TRACE("Scene::OnModelComponentAdded - Asset not ready yet, skipping material init");
            return;
        }
        
        Model &model = component.Asset->GetModel();
        
        // Protect manually loaded materials if they already exist
        if (component.Materials.empty())
        {
            CH_CORE_INFO("Scene::OnModelComponentAdded - Initializing materials from model");
            
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
                CH_CORE_INFO("Scene::OnModelComponentAdded - Created {} material slots", component.Materials.size());
            }
        }
        else
        {
            CH_CORE_INFO("Scene::OnModelComponentAdded - Keeping existing {} materials", component.Materials.size());
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
                    collider.BVHRoot = BVH::Build(modelAsset->GetModel(), MatrixIdentity());
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

Camera3D Scene::GetActiveCamera()
{
    // 1. Search for a primary camera component
    auto view = m_Registry.view<TransformComponent, CameraComponent>();
    for (auto entityHandle : view)
    {
        auto &cc = view.get<CameraComponent>(entityHandle);
        if (cc.Primary)
        {
            auto &tc = view.get<TransformComponent>(entityHandle);
            
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

            static entt::entity lastActive = entt::null;
            if (lastActive != entityHandle)
            {
                std::string name = m_Registry.any_of<TagComponent>(entityHandle) ? m_Registry.get<TagComponent>(entityHandle).Tag : "Unnamed Camera";
                CH_CORE_INFO("Active Scene Camera: '{}' (Entity ID: {})", name, (uint32_t)entityHandle);
                lastActive = entityHandle;
            }

            return camera;
        }
    }

    // 3. Absolute Fallback
    static bool warned = false;
    if (!warned)
    {
        CH_CORE_WARN("No Primary Camera found in scene! Using absolute fallback at (10, 10, 10). Please mark a CameraComponent as 'Primary'.");
        warned = true;
    }
    Camera3D camera = {0};
    camera.position = {10.0f, 10.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

Entity Scene::GetPrimaryCameraEntity()
{
    auto view = m_Registry.view<CameraComponent>();
    for (auto entity : view)
    {
        const auto& camera = view.get<CameraComponent>(entity);
        if (camera.Primary)
            return Entity{entity, this};
    }
    return {};
}

} // namespace CHEngine
