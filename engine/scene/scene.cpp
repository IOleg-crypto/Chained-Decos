#include "scene.h"
#include "components.h"
#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "entity.h"
#include "scriptable_entity.h"
#include <cfloat>

namespace CHEngine
{
Scene::Scene()
{
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
    if (!m_Registry.valid(entity))
    {
        CH_CORE_WARN("Attempted to destroy invalid entity: %d", (uint32_t)entity);
        return;
    }
    CH_CORE_INFO("Entity Destroyed: %d", (uint32_t)entity);
    m_Registry.destroy(entity);
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    // Native Scripting Logic
    {
        m_Registry.view<NativeScriptComponent>().each(
            [=](auto entity, auto &nsc)
            {
                for (auto &script : nsc.Scripts)
                {
                    if (!script.Instance)
                    {
                        script.Instance = script.InstantiateScript();
                        script.Instance->m_Entity = Entity{entity, this};
                        script.Instance->OnCreate();
                    }

                    script.Instance->OnUpdate(deltaTime);
                }
            });
    }

    // Animation Logic
    {
        m_Registry.view<AnimationComponent, ModelComponent>().each(
            [&](auto entity, auto &anim, auto &model)
            {
                if (anim.IsPlaying)
                {
                    Model rlModel = AssetManager::GetModel(model.ModelPath);
                    int animCount = 0;
                    ModelAnimation *animations =
                        AssetManager::GetAnimations(model.ModelPath, &animCount);

                    if (animations && anim.CurrentAnimationIndex < animCount)
                    {
                        anim.FrameTimeCounter += deltaTime;
                        float frameTime =
                            1.0f / 30.0f; // Standard 30fps for animations, or use anim speed

                        if (anim.FrameTimeCounter >= frameTime)
                        {
                            anim.CurrentFrame++;
                            anim.FrameTimeCounter = 0;

                            if (anim.CurrentFrame >=
                                animations[anim.CurrentAnimationIndex].frameCount)
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

                            UpdateModelAnimation(rlModel, animations[anim.CurrentAnimationIndex],
                                                 anim.CurrentFrame);
                        }
                    }
                }
            });
    }

    // (Old hardcoded player movement removed in favor of NativeScriptComponent)
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}

void Scene::OnEvent(Event &e)
{
    // Native Scripting Event Handling
    m_Registry.view<NativeScriptComponent>().each(
        [&](auto entity, auto &nsc)
        {
            for (auto &script : nsc.Scripts)
            {
                if (script.Instance)
                {
                    script.Instance->OnEvent(e);
                }
            }
        });
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

} // namespace CHEngine
