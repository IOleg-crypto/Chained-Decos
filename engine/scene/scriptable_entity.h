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

namespace CHEngine
{
class ScriptableEntity
{
public:
    virtual ~ScriptableEntity() = default;

    template <typename T> T &GetComponent()
    {
        return m_Entity.GetComponent<T>();
    }

    template <typename T> bool HasComponent()
    {
        return m_Entity.HasComponent<T>();
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

    virtual void OnCreate()
    {
    }
    virtual void OnDestroy()
    {
    }
    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnEvent(Event &e)
    {
    }

private:
    Entity m_Entity;
    friend class Scene;
};
} // namespace CHEngine

#endif // CH_SCRIPTABLE_ENTITY_H
