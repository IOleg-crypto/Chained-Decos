#ifndef CD_EDITOR_LOGIC_EDITOR_INPUT_H
#define CD_EDITOR_LOGIC_EDITOR_INPUT_H

#include "editor/editor_types.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include <functional>

namespace CHEngine
{
class EditorSceneActions;
class EditorEntityFactory;
class CommandHistory;
class SelectionManager;
class SceneSimulationManager;
class EditorSceneManager;
// Tool is included via editor_types.h

class EditorInput
{
public:
    struct Callbacks
    {
        std::function<void(Tool)> SetActiveTool;
    };

    EditorInput(EditorSceneActions *sceneActions, EditorEntityFactory *entityFactory,
                CommandHistory *commandHistory, SelectionManager *selectionManager,
                SceneSimulationManager *simulationManager, EditorSceneManager *sceneManager,
                Callbacks callbacks);

    bool OnEvent(Event &e);

private:
    bool OnKeyPressed(KeyPressedEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

private:
    EditorSceneActions *m_SceneActions;
    EditorEntityFactory *m_EntityFactory;
    CommandHistory *m_CommandHistory;
    SelectionManager *m_SelectionManager;
    SceneSimulationManager *m_SimulationManager;
    EditorSceneManager *m_SceneManager;
    Callbacks m_Callbacks;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_INPUT_H
