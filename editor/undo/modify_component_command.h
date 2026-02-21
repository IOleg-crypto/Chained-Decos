#ifndef CH_MODIFY_COMPONENT_COMMAND_H
#define CH_MODIFY_COMPONENT_COMMAND_H

#include "editor_command.h"
#include "engine/scene/scene.h"
#include <string>

namespace CHEngine
{

template <typename T> class ModifyComponentCommand : public IEditorCommand
{
public:
    ModifyComponentCommand(Entity entity, const T& oldState, const T& newState, const std::string& name = "")
        : m_Entity(entity),
          m_OldState(oldState),
          m_NewState(newState),
          m_Name(name)
    {
    }

    void Execute() override
    {
        if (Validate())
        {
            m_Entity.GetComponent<T>() = m_NewState;
        }
    }

    void Undo() override
    {
        if (Validate())
        {
            m_Entity.GetComponent<T>() = m_OldState;
        }
    }

    std::string GetName() const override
    {
        return m_Name.empty() ? "Modify Component" : m_Name;
    }

private:
    bool Validate()
    {
        if (!m_Entity)
        {
            return false;
        }
        auto* registry = &m_Entity.GetRegistry();
        return registry->valid(static_cast<entt::entity>(m_Entity)) && m_Entity.HasComponent<T>();
    }

private:
    Entity m_Entity;
    T m_OldState;
    T m_NewState;
    std::string m_Name;
};

} // namespace CHEngine

#endif // CH_MODIFY_COMPONENT_COMMAND_H
