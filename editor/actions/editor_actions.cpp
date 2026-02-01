#include "editor_actions.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/editor_layer.h"
#include "engine/core/input.h"
#include "engine/scene/scene.h"

namespace CHEngine
{

void EditorActions::PushCommand(std::unique_ptr<IEditorCommand> command)
{
    EditorLayer::GetCommandHistory().PushCommand(std::move(command));
}

bool EditorActions::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    bool handled = false;
    handled |= dispatcher.Dispatch<KeyPressedEvent>(CH_BIND_EVENT_FN(EditorActions::OnKeyPressed));
    handled |= dispatcher.Dispatch<MouseButtonPressedEvent>(
        CH_BIND_EVENT_FN(EditorActions::OnMouseButtonPressed));
    return handled;
}

bool EditorActions::OnKeyPressed(KeyPressedEvent &e)
{
    if (e.IsRepeat())
        return false;

    bool control = Input::IsKeyDown(KeyboardKey::KEY_LEFT_CONTROL) ||
                   Input::IsKeyDown(KeyboardKey::KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyDown(KeyboardKey::KEY_LEFT_SHIFT) ||
                 Input::IsKeyDown(KeyboardKey::KEY_RIGHT_SHIFT);

    switch (e.GetKeyCode())
    {
    case KeyboardKey::KEY_N:
        if (control)
        {
            SceneActions::New();
            return true;
        }
        break;
    case KeyboardKey::KEY_O:
        if (control)
        {
            SceneActions::Open();
            return true;
        }
        break;
    case KeyboardKey::KEY_S:
        if (control)
        {
            if (shift)
                SceneActions::SaveAs();
            else
                SceneActions::Save();
            return true;
        }
        break;
    case KeyboardKey::KEY_F5:
        ProjectActions::LaunchStandalone();
        return true;
    case KeyboardKey::KEY_Z:
        if (control)
        {
            EditorLayer::GetCommandHistory().Undo();
            return true;
        }
        break;
    case KeyboardKey::KEY_Y:
        if (control)
        {
            EditorLayer::GetCommandHistory().Redo();
            return true;
        }
        break;
    }

    return false;
}

bool EditorActions::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    return false;
}

} // namespace CHEngine
