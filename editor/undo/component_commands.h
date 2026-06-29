#ifndef CH_COMPONENT_COMMANDS_H
#define CH_COMPONENT_COMMANDS_H

#include "command.h"
#include "engine/scene/scene.h"
#include <string>

namespace Chained
{

template <typename T> class AddComponentCommand : public IEditorCommand
{
public:
    AddComponentCommand(Entity entity)
        : m_Entity(entity)
    {
    }

    void Execute() override
    {
        if (Validate() && !m_Entity.HasComponent<T>())
        {
            m_Entity.AddComponent<T>();
        }
    }

    void Undo() override
    {
        if (Validate() && m_Entity.HasComponent<T>())
        {
            m_Entity.RemoveComponent<T>();
        }
    }

    std::string GetName() const override
    {
        return "Add Component";
    }

private:
    bool Validate()
    {
        if (!m_Entity)
        {
            return false;
        }
        auto* registry = &m_Entity.GetRegistry();
        return registry->valid(static_cast<entt::entity>(m_Entity));
    }

    Entity m_Entity;
};

template <typename T> class RemoveComponentCommand : public IEditorCommand
{
public:
    RemoveComponentCommand(Entity entity)
        : m_Entity(entity),
          m_ComponentState(entity.GetComponent<T>())
    {
    }

    void Execute() override
    {
        if (Validate() && m_Entity.HasComponent<T>())
        {
            m_Entity.RemoveComponent<T>();
        }
    }

    void Undo() override
    {
        if (Validate() && !m_Entity.HasComponent<T>())
        {
            m_Entity.AddComponent<T>(m_ComponentState);
        }
    }

    std::string GetName() const override
    {
        return "Remove Component";
    }

private:
    bool Validate()
    {
        if (!m_Entity)
        {
            return false;
        }
        auto* registry = &m_Entity.GetRegistry();
        return registry->valid(static_cast<entt::entity>(m_Entity));
    }

    Entity m_Entity;
    T m_ComponentState;
};

} // namespace Chained

#endif // CH_COMPONENT_COMMANDS_H
