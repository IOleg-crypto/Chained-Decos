#ifndef EDITOR_INPUT_H
#define EDITOR_INPUT_H

#include "editor/EditorTypes.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include <functional>

namespace CHEngine
{
class EditorSceneActions;
class EditorEntityFactory;
class CommandHistory;
class SelectionManager;
class SceneSimulationManager;
// Tool is included via EditorTypes.h

class EditorInput
{
public:
    struct Callbacks
    {
        std::function<void(Tool)> SetActiveTool;
    };

    EditorInput(EditorSceneActions *sceneActions, EditorEntityFactory *entityFactory,
                CommandHistory *commandHistory, SelectionManager *selectionManager,
                SceneSimulationManager *simulationManager, Callbacks callbacks);

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
    Callbacks m_Callbacks;
};
} // namespace CHEngine

#endif // EDITOR_INPUT_H
