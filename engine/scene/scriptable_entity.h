#ifndef CH_SCRIPTABLE_ENTITY_H
#define CH_SCRIPTABLE_ENTITY_H

#include "engine/core/events.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_events.h"
#include "engine/core/application.h"
#include "engine/scene/scene.h"
#include "engine/core/timestep.h"

#define CH_SCRIPT(name) class name : public CHEngine::ScriptableEntity
#define CH_START() void OnCreate() override
#define CH_UPDATE(dt) void OnUpdate(CHEngine::Timestep dt) override
#define CH_EVENT(e) void OnEvent(CHEngine::Event &e) override
#define CH_GUI() void OnImGuiRender() override

namespace CHEngine
{
class Scene;

class ScriptableEntity
{
public:
    virtual ~ScriptableEntity() = default;

    template <typename T> T &GetComponent()
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
    virtual void OnEvent(Event &e)
    {
    }

protected:
    Entity &GetEntity()
    {
        return m_Entity;
    }

    // Convenience Getters
    TransformComponent &Transform()
    {
        return GetComponent<TransformComponent>();
    }
    Vector3 &Translation()
    {
        return Transform().Translation;
    }
    Vector3 &Rotation()
    {
        return Transform().Rotation;
    }
    Vector3 &Scale()
    {
        return Transform().Scale;
    }

    RigidBodyComponent &RigidBody()
    {
        return GetComponent<RigidBodyComponent>();
    }
    Vector3 &Velocity()
    {
        return RigidBody().Velocity;
    }

    Entity FindEntityByTag(const std::string &tag)
    {
        return m_Entity.GetScene()->FindEntityByTag(tag);
    }

    Entity FindEntityByUUID(UUID uuid)
    {
        return m_Entity.GetScene()->GetEntityByUUID(uuid);
    }

    bool IsButtonClicked(const std::string &tagName)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<ButtonControl>())
            return e.GetComponent<ButtonControl>().PressedThisFrame;
        return false;
    }

    bool IsUIControlActive(const std::string &tagName)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<ControlComponent>())
            return e.GetComponent<ControlComponent>().IsActive;
        return false;
    }

    void SetUIControlActive(const std::string &tagName, bool active)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<ControlComponent>())
            e.GetComponent<ControlComponent>().IsActive = active;
    }

    void SetUIControlValue(const std::string &tagName, float value)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<SliderControl>())
            e.GetComponent<SliderControl>().Value = value;
    }

    float GetUIControlValue(const std::string &tagName)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<SliderControl>())
            return e.GetComponent<SliderControl>().Value;
        return 0.0f;
    }

    void ChangeScene(const std::string &path)
    {
        SceneChangeRequestEvent e(path);
        Application::Get().OnEvent(e);
    }

    ScriptableEntity *GetScript(const std::string &name)
    {
        if (HasComponent<NativeScriptComponent>())
        {
            auto &nsc = GetComponent<NativeScriptComponent>();
            for (auto &script : nsc.Scripts)
            {
                if (script.ScriptName == name)
                    return script.Instance;
            }
        }
        return nullptr;
    }

private:
    Entity m_Entity;
    friend class Scene;
    friend class SceneScripting;
};
} // namespace CHEngine

#endif // CH_SCRIPTABLE_ENTITY_H
