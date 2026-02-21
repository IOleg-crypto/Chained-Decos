#ifndef CH_SCRIPTABLE_ENTITY_H
#define CH_SCRIPTABLE_ENTITY_H

#include "engine/core/application.h"
#include "engine/core/events.h"
#include "engine/core/log.h"
#include "engine/core/timestep.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"

#define CH_SCRIPT(name) class name : public CHEngine::ScriptableEntity
#define CH_START() void OnCreate() override
#define CH_UPDATE(dt) void OnUpdate(CHEngine::Timestep dt) override
#define CH_EVENT(e) void OnEvent(CHEngine::Event& e) override
#define CH_GUI() void OnImGuiRender() override

namespace CHEngine
{

class ScriptableEntity
{
public:
    virtual ~ScriptableEntity() = default;

    virtual void OnCreate()
    {
    }
    virtual void OnDestroy()
    {
    }
    virtual void OnUpdate(Timestep deltaTime)
    {
    }
    virtual void OnImGuiRender()
    {
    }
    virtual void OnEvent(Event& e)
    {
    }

protected:
    // Core entity access
    Entity& GetEntity()
    {
        return m_Entity;
    }
    Scene* GetScene()
    {
        return m_Scene;
    }

    // Component access helpers
    template <typename T> T& GetComponent()
    {
        if (!m_Entity.HasComponent<T>())
        {
            CH_CORE_ERROR("ScriptableEntity: Entity '{0}' does not have component {1}!",
                          m_Entity.GetComponent<TagComponent>().Tag, typeid(T).name());
        }
        return m_Entity.GetComponent<T>();
    }

    template <typename T> bool HasComponent()
    {
        return m_Entity.HasComponent<T>();
    }

    template <typename T, typename... Args> T& AddComponent(Args&&... args)
    {
        return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
    }

    template <typename T> void RemoveComponent()
    {
        m_Entity.RemoveComponent<T>();
    }

private:
    Entity m_Entity;
    Scene* m_Scene = nullptr;
    friend class Scene;
    friend class SceneScripting;
};
} // namespace CHEngine

#endif // CH_SCRIPTABLE_ENTITY_H
