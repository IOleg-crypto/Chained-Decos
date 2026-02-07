#ifndef CH_ENTITY_COMMANDS_H
#define CH_ENTITY_COMMANDS_H

#include "editor_command.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class DestroyEntityCommand : public IEditorCommand
{
public:
    DestroyEntityCommand(Entity entity) : m_Entity(entity), m_Scene(entity.GetScene())
    {
    }

    void Execute() override
    {
        CH_CORE_INFO("Destroying entity via command: {}",
                     m_Entity.GetComponent<TagComponent>().Tag);
        m_Scene->DestroyEntity(m_Entity);
    }

    void Undo() override
    {
        CH_CORE_WARN("Undo DestroyEntity not fully implemented yet (requires restoration)");
    }

    std::string GetName() const override
    {
        return "Destroy Entity";
    }

private:
    Entity m_Entity;
    Scene *m_Scene;
};

class CreateEntityCommand : public IEditorCommand
{
public:
    CreateEntityCommand(Scene *scene, const std::string &name, const std::string &modelPath = "")
        : m_Scene(scene), m_Name(name), m_ModelPath(modelPath)
    {
    }

    void Execute() override
    {
        m_Entity = m_Scene->CreateEntity(m_Name);
        if (!m_ModelPath.empty())
        {
            auto &mc = m_Entity.AddComponent<ModelComponent>();
            mc.ModelPath = m_ModelPath;
        }
    }

    void Undo() override
    {
        if (m_Entity)
            m_Scene->DestroyEntity(m_Entity);
    }

    std::string GetName() const override
    {
        return "Create Entity";
    }

private:
    Scene *m_Scene;
    std::string m_Name;
    std::string m_ModelPath;
    Entity m_Entity;
};
} // namespace CHEngine

#endif // CH_ENTITY_COMMANDS_H
