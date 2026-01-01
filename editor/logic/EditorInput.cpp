#include "EditorInput.h"
#include "core/Base.h"
#include "core/input/Input.h"
#include "editor/EditorTypes.h"
#include "editor/logic/EditorEntityFactory.h"
#include "editor/logic/EditorSceneActions.h"
#include "editor/logic/SceneSimulationManager.h"
#include "editor/logic/SelectionManager.h"
#include "editor/logic/undo/CommandHistory.h"
#include "scene/MapManager.h"

namespace CHEngine
{

EditorInput::EditorInput(EditorSceneActions *sceneActions, EditorEntityFactory *entityFactory,
                         CommandHistory *commandHistory, SelectionManager *selectionManager,
                         SceneSimulationManager *simulationManager, Callbacks callbacks)
    : m_SceneActions(sceneActions), m_EntityFactory(entityFactory),
      m_CommandHistory(commandHistory), m_SelectionManager(selectionManager),
      m_SimulationManager(simulationManager), m_Callbacks(callbacks)
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
            if (control) // Ctrl + Ctrl + N? Wait, EditorLayer logic had 'if (control)' twice.
                         // Actually e.IsControlDown() would be better but we use Input::IsKeyDown.
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
            SelectionType type = m_SelectionManager->GetSelectionType();
            if (type == SelectionType::ENTITY)
            {
                m_EntityFactory->DeleteEntity(m_SelectionManager->GetSelectedEntity());
            }
            else if (type == SelectionType::WORLD_OBJECT)
            {
                m_EntityFactory->DeleteObject(m_SelectionManager->GetSelectedIndex());
            }
            else if (type == SelectionType::UI_ELEMENT)
            {
                auto activeScene = MapManager::GetCurrentScene();
                if (m_SelectionManager->GetSelectedIndex() >= 0 && activeScene)
                {
                    auto &elements = activeScene->GetUIElementsMutable();
                    if (m_SelectionManager->GetSelectedIndex() < (int)elements.size())
                    {
                        elements.erase(elements.begin() + m_SelectionManager->GetSelectedIndex());
                        m_SelectionManager->ClearSelection();
                    }
                }
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
