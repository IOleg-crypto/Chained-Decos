#include "editor/editor_types.h"
#include "editor_panel.h"
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
    ToolbarPanel();
    virtual ~ToolbarPanel() = default;

    virtual void OnImGuiRender() override;
};
} // namespace CHEngine
