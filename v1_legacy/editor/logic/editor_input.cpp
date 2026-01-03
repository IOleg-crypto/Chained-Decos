#include "editor_input.h"
#include "editor/editor_types.h"
#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/editor_scene_manager.h"
#include "editor/logic/scene_simulation_manager.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"
#include "engine/core/input/input.h"


namespace CHEngine
{

EditorInput::EditorInput()
{
}

bool EditorInput::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    return dispatcher.Dispatch<KeyPressedEvent>(CD_BIND_EVENT_FN(EditorInput::OnKeyPressed));
}

bool EditorInput::OnKeyPressed(KeyPressedEvent &e)
{
    if (e.GetRepeatCount() > 0)
        return false;

    bool control = Input::IsKeyDown(KEY_LEFT_CONTROL) || Input::IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyDown(KEY_LEFT_SHIFT) || Input::IsKeyDown(KEY_RIGHT_SHIFT);

    // Editor Shortcuts
    if (control)
    {
        switch (e.GetKeyCode())
        {
        case KEY_N:
            SceneManager::Get().NewScene();
            break;
        case KEY_O: // This would trigger UI - maybe call EditorLayer's Actions?
            break;
        case KEY_S:
            if (shift)
                SceneManager::Get().SaveSceneAs(""); // Needs path logic
            else
                SceneManager::Get().SaveScene();
            break;
        }
    }

    return false;
}

} // namespace CHEngine
