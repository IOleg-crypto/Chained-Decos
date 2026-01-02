#ifndef CD_EDITOR_PANELS_MENU_BAR_PANEL_H
#define CD_EDITOR_PANELS_MENU_BAR_PANEL_H

#include "editor_panel.h"
#include <memory>

namespace CHEngine
{
class EditorSceneActions;
class EditorProjectActions;
class CommandHistory;
class PanelManager;

class MenuBarPanel : public EditorPanel
{
public:
    MenuBarPanel(EditorSceneActions *sceneActions, EditorProjectActions *projectActions,
                 CommandHistory *commandHistory, PanelManager *panelManager,
                 bool *showProjectSettings);
    virtual ~MenuBarPanel() = default;

    virtual void OnImGuiRender() override;

private:
    EditorSceneActions *m_SceneActions;
    EditorProjectActions *m_ProjectActions;
    CommandHistory *m_CommandHistory;
    PanelManager *m_PanelManager;
    bool *m_ShowProjectSettings;
};
} // namespace CHEngine

#endif // CD_EDITOR_PANELS_MENU_BAR_PANEL_H
