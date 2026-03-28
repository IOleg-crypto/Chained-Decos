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

bool EditorActions::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    bool handled = false;
    handled |= dispatcher.Dispatch<KeyPressedEvent>(CH_BIND_EVENT_FN(EditorActions::OnKeyPressed));
    handled |= dispatcher.Dispatch<MouseButtonPressedEvent>(CH_BIND_EVENT_FN(EditorActions::OnMouseButtonPressed));
    return handled;
}

bool EditorActions::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.IsRepeat())
    {
        return false;
    }

    bool ctrl = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
    bool shift = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

    auto keyCode = e.GetKeyCode();

    if (ctrl)
    {
        switch (keyCode)
        {
        case Key::N:
            SceneActions::New();
            return true;
        case Key::O:
            SceneActions::Open();
            return true;
        case Key::S:
            if (shift)
            {
                SceneActions::SaveAs();
            }
            else
            {
                SceneActions::Save();
            }
            return true;
        case Key::Z:
            EditorLayer::GetCommandHistory().Undo();
            return true;
        case Key::Y:
            EditorLayer::GetCommandHistory().Redo();
            return true;
        }
    }

    if (keyCode == Key::F5)
    {
        ProjectActions::LaunchStandalone();
        return true;
    }

    return false;
}

bool EditorActions::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    return false;
}

} // namespace CHEngine
