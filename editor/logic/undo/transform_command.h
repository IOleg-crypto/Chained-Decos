#ifndef CH_TRANSFORM_COMMAND_H
#define CH_TRANSFORM_COMMAND_H

#include "editor_command.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"

namespace CH
{
/**
 * @brief Command for undoing/redoing transform changes
 */
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
        if (m_Entity && m_Entity.HasComponent<TransformComponent>())
        {
            m_Entity.GetComponent<TransformComponent>() = m_NewTransform;
        }
    }

    void Undo() override
    {
        if (m_Entity && m_Entity.HasComponent<TransformComponent>())
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
} // namespace CH

#endif // CH_TRANSFORM_COMMAND_H
