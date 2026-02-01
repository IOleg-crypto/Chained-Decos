#ifndef CH_EDITOR_ACTIONS_H
#define CH_EDITOR_ACTIONS_H

#include "editor/undo/editor_command.h"
#include "editor/undo/modify_component_command.h"
#include "engine/core/events.h"
#include <functional>
#include <memory>

namespace CHEngine
{
class EditorActions
{
public:
    EditorActions() = default;

    static void PushCommand(std::unique_ptr<IEditorCommand> command);

    template <typename T, typename F>
    static void ModifyComponent(Entity entity, const std::string &name, F &&modifier)
    {
        if (!entity || !entity.HasComponent<T>())
            return;

        T oldState = entity.GetComponent<T>();
        modifier(entity.GetComponent<T>());
        T newState = entity.GetComponent<T>();

        PushCommand(std::make_unique<ModifyComponentCommand<T>>(entity, oldState, newState, name));
    }

    bool OnEvent(Event &e);

private:
    bool OnKeyPressed(KeyPressedEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);
};

} // namespace CHEngine

#endif // CH_EDITOR_ACTIONS_H
