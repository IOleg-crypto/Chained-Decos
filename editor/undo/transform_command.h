#ifndef CH_TRANSFORM_COMMAND_H
#define CH_TRANSFORM_COMMAND_H

#include "editor_command.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"

namespace CHEngine
{
class TransformCommand : public IEditorCommand
{
public:
    TransformCommand(Entity entity, const TransformComponent &oldTransform,
                     const TransformComponent &newTransform)
        : m_Entity(entity), m_OldTransform(oldTransform), m_NewTransform(newTransform)
    {
    }

    void Execute() override
    {
        // Check if entity is still valid before accessing
        if (m_Entity && m_Entity.GetScene() &&
            m_Entity.GetScene()->GetRegistry().valid(static_cast<entt::entity>(m_Entity)) &&
            m_Entity.HasComponent<TransformComponent>())
        {
            m_Entity.GetComponent<TransformComponent>() = m_NewTransform;
        }
    }

    void Undo() override
    {
        // Check if entity is still valid before accessing
        if (m_Entity && m_Entity.GetScene() &&
            m_Entity.GetScene()->GetRegistry().valid(static_cast<entt::entity>(m_Entity)) &&
            m_Entity.HasComponent<TransformComponent>())
        {
            m_Entity.GetComponent<TransformComponent>() = m_OldTransform;
        }
    }

    std::string GetName() const override
    {
        return "Transform Entity";
    }

private:
    Entity m_Entity;
    TransformComponent m_OldTransform;
    TransformComponent m_NewTransform;
};
} // namespace CHEngine

#endif // CH_TRANSFORM_COMMAND_H
