#include "scene.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "scene_animator.h"
#include "scene_audio.h"
#include "scene_scripting.h"
#include "scene_serializer.h"
#include "scriptable_entity.h"

namespace CHEngine
{
Scene::Scene()
{
    m_Scripting = CreateScope<SceneScripting>(this);
    m_Animator = CreateScope<SceneAnimator>(this);
    m_Audio = CreateScope<SceneAudio>(this);

    m_Registry.on_construct<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);
    m_Registry.on_update<ModelComponent>().connect<&Scene::OnModelComponentAdded>(this);

    m_Registry.on_construct<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);
    m_Registry.on_update<AnimationComponent>().connect<&Scene::OnAnimationComponentAdded>(this);

    m_Registry.on_construct<AudioComponent>().connect<&Scene::OnAudioComponentAdded>(this);
    m_Registry.on_update<AudioComponent>().connect<&Scene::OnAudioComponentAdded>(this);
}

Scene::~Scene()
{
}

Ref<Scene> Scene::Copy(Ref<Scene> other)
{
    Ref<Scene> newScene = CreateRef<Scene>();

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

    CH_CORE_INFO("Entity Created: %s (%llu)", name,
                 (uint64_t)entity.GetComponent<IDComponent>().ID);
    return entity;
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created with UUID: %s (%llu)", name, (uint64_t)uuid);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!entity || !m_Registry.valid(entity))
    {
        CH_CORE_WARN("Attempted to destroy invalid entity");
        return;
    }

    if (entity.HasComponent<HierarchyComponent>())
    {
        auto &hc = entity.GetComponent<HierarchyComponent>();
        if (hc.Parent != entt::null)
        {
            Entity parent{hc.Parent, this};
            if (parent.HasComponent<HierarchyComponent>())
            {
                auto &phc = parent.GetComponent<HierarchyComponent>();
                auto it = std::find(phc.Children.begin(), phc.Children.end(), (entt::entity)entity);
                if (it != phc.Children.end())
                    phc.Children.erase(it);
            }
        }

        std::vector<entt::entity> children = hc.Children;
        for (auto child : children)
        {
            DestroyEntity({child, this});
        }
    }

    CH_CORE_INFO("Entity Destroyed: %d", (uint32_t)entity);
    m_Registry.destroy(entity);
}

// (Old hardcoded player movement removed in favor of NativeScriptComponent)

template <> void Scene::OnComponentAdded<ModelComponent>(Entity entity, ModelComponent &component)
{
    if (!component.ModelPath.empty())
    {
        // Check if we need to (re)load the asset
        bool pathChanged = !component.Asset || (component.Asset->GetPath() != component.ModelPath);

        if (pathChanged)
        {
            component.Asset = Assets::Get<ModelAsset>(component.ModelPath);
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
            mc.Asset = Assets::Get<ModelAsset>(mc.ModelPath);
        }
    }
}

template <> void Scene::OnComponentAdded<AudioComponent>(Entity entity, AudioComponent &component)
{
    if (!component.SoundPath.empty())
    {
        component.Asset = Assets::Get<SoundAsset>(component.SoundPath);
    }
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    CH_PROFILE_FUNCTION();

    m_Scripting->OnUpdate(deltaTime);
    m_Animator->OnUpdate(deltaTime);
    m_Audio->OnUpdate(deltaTime);
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}

void Scene::OnRuntimeStart()
{
    m_IsSimulationRunning = true;
    CH_CORE_INFO("Scene '{0}' simulation started.",
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
    m_Scripting->OnEvent(e);
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
    auto view = m_Registry.view<IDComponent>();
    for (auto entity : view)
    {
        if (view.get<IDComponent>(entity).ID == uuid)
            return {entity, this};
    }
    return {};
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

} // namespace CHEngine
