#include "editor_panel.h"
#include "editor/editor_types.h"
#include <functional>

namespace CHEngine
{
class EditorSceneActions;
class SceneSimulationManager;

/**
 * @brief Top toolbar for quick scene actions and tool selection
 */
class ToolbarPanel : public EditorPanel
{
public:
    ToolbarPanel(EditorSceneActions *sceneActions, SceneSimulationManager *simulationManager,
                 Tool *activeTool);
    virtual ~ToolbarPanel() = default;

    virtual void OnImGuiRender() override;

private:
    EditorSceneActions *m_SceneActions = nullptr;
    SceneSimulationManager *m_SimulationManager = nullptr;
    Tool *m_ActiveTool = nullptr;
};
} // namespace CHEngine
