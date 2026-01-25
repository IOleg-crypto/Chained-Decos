#ifndef CH_SCRIPTABLE_ENTITY_H
#define CH_SCRIPTABLE_ENTITY_H

#include "engine/core/events.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"

#define CH_SCRIPT(name) class name : public CHEngine::ScriptableEntity
#define CH_START() void OnCreate() override
#define CH_UPDATE(dt) void OnUpdate(float dt) override
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
    virtual void OnUpdate(float deltaTime)
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
        if (e && e.HasComponent<ButtonWidget>())
            return e.GetComponent<ButtonWidget>().Pressed;
        return false;
    }

    bool IsUIActive(const std::string &tagName)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<WidgetComponent>())
            return e.GetComponent<WidgetComponent>().IsActive;
        return false;
    }

    void SetUIActive(const std::string &tagName, bool active)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<WidgetComponent>())
            e.GetComponent<WidgetComponent>().IsActive = active;
    }

    void SetUIValue(const std::string &tagName, float value)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<SliderWidget>())
            e.GetComponent<SliderWidget>().Value = value;
    }

    float GetUIValue(const std::string &tagName)
    {
        Entity e = m_Entity.GetScene()->FindEntityByTag(tagName);
        if (e && e.HasComponent<SliderWidget>())
            return e.GetComponent<SliderWidget>().Value;
        return 0.0f;
    }

    void ChangeScene(const std::string &path)
    {
        m_Entity.GetScene()->RequestSceneChange(path);
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
