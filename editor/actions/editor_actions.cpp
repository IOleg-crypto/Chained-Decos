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

        bool ctrl = Input::IsKeyDown(KeyboardKey::KEY_LEFT_CONTROL) || Input::IsKeyDown(KeyboardKey::KEY_RIGHT_CONTROL);
        bool shift = Input::IsKeyDown(KeyboardKey::KEY_LEFT_SHIFT) || Input::IsKeyDown(KeyboardKey::KEY_RIGHT_SHIFT);
        bool alt = Input::IsKeyDown(KeyboardKey::KEY_LEFT_ALT) || Input::IsKeyDown(KeyboardKey::KEY_RIGHT_ALT);

        struct Shortcut { KeyboardKey Key; bool Ctrl, Shift, Alt; std::function<void()> Action; };
        
        static const Shortcut shortcuts[] = {
            { KEY_N,  true,  false, false, []() { SceneActions::New(); } },
            { KEY_O,  true,  false, false, []() { SceneActions::Open(); } },
            { KEY_S,  true,  false, false, []() { SceneActions::Save(); } },
            { KEY_S,  true,  true,  false, []() { SceneActions::SaveAs(); } },
            { KEY_Z,  true,  false, false, []() { EditorLayer::GetCommandHistory().Undo(); } },
            { KEY_Y,  true,  false, false, []() { EditorLayer::GetCommandHistory().Redo(); } },
            { KEY_F5, false, false, false, []() { ProjectActions::LaunchStandalone(); } }
        };

        for (auto& s : shortcuts)
        {
            if (e.GetKeyCode() == s.Key && ctrl == s.Ctrl && shift == s.Shift && alt == s.Alt)
            {
                s.Action();
                return true;
            }
        }

        return false;
    }

bool EditorActions::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    return false;
}

} // namespace CHEngine
