#include "engine/scene/scene.h"
#include "engine/audio/audio.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include <cmath>
#include "serialization_utils.h"
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

void Scene::OnRuntimeStart()
{
    Physics::ResetAccumulator(this);
    m_IsSimulationRunning = true;
}

void Scene::OnRuntimeStop()
{
    m_IsSimulationRunning = false;
    Physics::ClearContext(this);
}

void Scene::OnUpdateRuntime(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateHierarchy();
    UpdateAnimations(timestep);
    UpdatePhysics(timestep);
    UpdateAudio(timestep);
}

void Scene::OnUpdateEditor(Timestep timestep)
{
    CH_PROFILE_FUNCTION();

    UpdateHierarchy();
    UpdateAnimations(timestep);
    UpdatePhysics(timestep);
    UpdateAudio(timestep);
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

void Scene::UpdatePhysics(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();
    Physics::Update(this, deltaTime, m_IsSimulationRunning);
}

void Scene::UpdateAnimations(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();
    auto& reg = GetRegistry();
    auto view = reg.view<AnimationComponent, ModelComponent>();

    for (auto entity : view)
    {
        auto& animation = view.get<AnimationComponent>(entity);
        if (!animation.IsPlaying)
        {
            continue;
        }

        auto& model = view.get<ModelComponent>(entity);
        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetAnimationCount() == 0)
        {
            continue;
        }

        // Progress timers
        float dt = deltaTime.GetSeconds();
        animation.FrameTimeCounter += dt;

        // Get animation frameRate from asset (asset-driven, defaults to 30fps if not available)
        float targetFPS = 30.0f;
        const auto& rawAnims = modelAsset->GetAnimations();
        if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
        {
            targetFPS = rawAnims[animation.CurrentAnimationIndex].frameRate;
        }
        float frameTime = 1.0f / targetFPS;

        // Use a while loop to correctly consume elapsed time without dropping fractional MS,
        // which prevents animation sequence shaking/jittering.
        while (animation.FrameTimeCounter >= frameTime)
        {
            animation.FrameTimeCounter -= frameTime;
            animation.CurrentFrame++;

            if (animation.CurrentAnimationIndex >= 0 && animation.CurrentAnimationIndex < (int)rawAnims.size())
            {
                int totalFrames = rawAnims[animation.CurrentAnimationIndex].frameCount;
                if (animation.CurrentFrame >= totalFrames)
                {
                    if (animation.IsLooping)
                    {
                        animation.CurrentFrame = 0;
                    }
                    else
                    {
                        animation.CurrentFrame = totalFrames - 1;
                        animation.IsPlaying = false;
                    }
                }
            }
        }

        // Handle Blending
        if (animation.Blending)
        {
            animation.BlendTimer += dt;
            if (animation.BlendTimer >= animation.BlendDuration)
            {
                // Blend complete
                animation.CurrentAnimationIndex = animation.TargetAnimationIndex;
                animation.CurrentFrame = animation.TargetFrame;
                animation.Blending = false;
                animation.TargetAnimationIndex = -1;
            }
            else
            {
                // Progress target frame too
                const float FRAME_EPSILON = 0.001f;
                if (std::abs(animation.FrameTimeCounter - 0.0f) < FRAME_EPSILON) // Just advanced a frame
                {
                    animation.TargetFrame++;
                    if (animation.TargetAnimationIndex >= 0 &&
                        animation.TargetAnimationIndex < modelAsset->GetAnimationCount())
                    {
                        int targetTotalFrames =
                            modelAsset->GetAnimations()[animation.TargetAnimationIndex].frameCount;
                        if (animation.TargetFrame >= targetTotalFrames)
                        {
                            animation.TargetFrame = 0;
                        }
                    }
                }
            }
        }
    }
}

void Scene::UpdateHierarchy()
{
    CH_PROFILE_FUNCTION();
    auto& reg = GetRegistry();
    auto view = reg.view<TransformComponent>();

    struct UpdateTask
    {
        entt::entity Entity;
        glm::mat4 ParentTransform;
        bool ParentChanged;
    };

    std::vector<UpdateTask> stack;
    stack.reserve(reg.storage<entt::entity>().size());

    // 1. Find all root entities and push to stack
    for (auto entity : view)
    {
        bool isRoot = true;
        if (reg.all_of<HierarchyComponent>(entity))
        {
            auto& hc = reg.get<HierarchyComponent>(entity);
            if (hc.Parent != entt::null && reg.valid(hc.Parent) && reg.all_of<TransformComponent>(hc.Parent))
            {
                isRoot = false;
            }
        }

        if (isRoot)
        {
            stack.push_back({entity, glm::mat4(1.0f), false});
        }
    }

    // 2. Iterative DFS update with dirty flag propagation
    while (!stack.empty())
    {
        UpdateTask task = stack.back();
        stack.pop_back();

        auto& tc = view.get<TransformComponent>(task.Entity);
        
        // A node needs update if it is explicitly dirty OR its parent's world transform changed
        bool needsUpdate = task.ParentChanged || tc.IsDirty;
        
        if (needsUpdate)
        {
            tc.WorldTransform = task.ParentTransform * tc.GetTransform();
            tc.IsDirty = false;
        }

        if (reg.all_of<HierarchyComponent>(task.Entity))
        {
            auto& hc = reg.get<HierarchyComponent>(task.Entity);
            for (auto child : hc.Children)
            {
                if (reg.valid(child) && reg.all_of<TransformComponent>(child))
                {
                    stack.push_back({child, tc.WorldTransform, needsUpdate});
                }
            }
        }
    }
}

void Scene::UpdateAudio(Timestep deltaTime)
{
    CH_PROFILE_FUNCTION();

    // 1. Sync Listener with Primary Camera
    auto cameraView = GetRegistry().view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto& camera = cameraView.get<CameraComponent>(entity);
        if (camera.Primary)
        {
            auto& transform = cameraView.get<TransformComponent>(entity);
            glm::vec3 pos = glm::vec3(transform.WorldTransform[3]);
            glm::mat3 rot = glm::mat3(transform.WorldTransform);
            glm::vec3 forward = rot * glm::vec3(0, 0, -1);
            glm::vec3 up = rot * glm::vec3(0, 1, 0);

            Audio::Get().SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = GetRegistry().view<AudioComponent, TransformComponent>();
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);
        if (audio.PlayOnStart && !audio.IsPlaying && !audio.SoundPath.empty())
        {
            if (audio.SoundHandle == 0 || !Audio::Get().IsSoundLoaded(audio.SoundHandle))
            {
                audio.SoundHandle = Audio::Get().LoadSound(audio.SoundPath);
            }
            
            if (audio.SoundHandle != 0)
            {
                auto& transform = audioView.get<TransformComponent>(entity);
                glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

                Audio::Get().Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized,
                                  worldPos);
                audio.IsPlaying = true;
            }
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
