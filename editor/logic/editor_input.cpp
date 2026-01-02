#include "editor_input.h"
#include "core/input/input.h"
#include "editor/editor_types.h"
#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/editor_scene_manager.h"
#include "editor/logic/scene_simulation_manager.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"

namespace CHEngine
{

EditorInput::EditorInput(EditorSceneActions *sceneActions, EditorEntityFactory *entityFactory,
                         CommandHistory *commandHistory, SelectionManager *selectionManager,
                         SceneSimulationManager *simulationManager,
                         EditorSceneManager *sceneManager, Callbacks callbacks)
    : m_SceneActions(sceneActions), m_EntityFactory(entityFactory),
      m_CommandHistory(commandHistory), m_SelectionManager(selectionManager),
      m_SimulationManager(simulationManager), m_SceneManager(sceneManager), m_Callbacks(callbacks)
{
}

bool EditorInput::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    bool handled = false;
    handled |= dispatcher.Dispatch<KeyPressedEvent>(CD_BIND_EVENT_FN(EditorInput::OnKeyPressed));
    handled |= dispatcher.Dispatch<MouseButtonPressedEvent>(
        CD_BIND_EVENT_FN(EditorInput::OnMouseButtonPressed));
    return handled;
}

bool EditorInput::OnKeyPressed(KeyPressedEvent &e)
{
    if (e.GetRepeatCount() > 0)
        return false;

    bool control = Input::IsKeyDown(KEY_LEFT_CONTROL) || Input::IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyDown(KEY_LEFT_SHIFT) || Input::IsKeyDown(KEY_RIGHT_SHIFT);

    if (control)
    {
        switch (e.GetKeyCode())
        {
        case KEY_N:
            if (control)
                m_SceneActions->NewScene();
            else
                m_EntityFactory->CreateEntity();
            break;
        case KEY_O:
            m_SceneActions->OpenScene();
            break;
        case KEY_S:
            if (shift)
                m_SceneActions->SaveSceneAs();
            else
                m_SceneActions->SaveScene();
            break;
        case KEY_Z:
            m_CommandHistory->Undo();
            break;
        case KEY_Y:
            m_CommandHistory->Redo();
            break;
        }
    }
    else
    {
        switch (e.GetKeyCode())
        {
        case KEY_ESCAPE:
            if (m_SimulationManager->GetSceneState() == SceneState::Play)
                m_SceneActions->OnSceneStop();
            break;
        case KEY_DELETE:
        {
            if (m_SelectionManager->HasSelection())
            {
                m_EntityFactory->DeleteEntity(m_SelectionManager->GetSelectedEntity());
            }
            break;
        }
        case KEY_Q:
            if (m_SimulationManager->GetSceneState() == SceneState::Edit)
                if (m_Callbacks.SetActiveTool)
                    m_Callbacks.SetActiveTool(Tool::SELECT);
            break;
        case KEY_W:
            if (m_SimulationManager->GetSceneState() == SceneState::Edit)
                if (m_Callbacks.SetActiveTool)
                    m_Callbacks.SetActiveTool(Tool::MOVE);
            break;
        case KEY_E:
            if (m_SimulationManager->GetSceneState() == SceneState::Edit)
                if (m_Callbacks.SetActiveTool)
                    m_Callbacks.SetActiveTool(Tool::ROTATE);
            break;
        case KEY_R:
            if (m_SimulationManager->GetSceneState() == SceneState::Edit)
                if (m_Callbacks.SetActiveTool)
                    m_Callbacks.SetActiveTool(Tool::SCALE);
            break;
        }
    }

    return false;
}

bool EditorInput::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    return false;
}

} // namespace CHEngine
