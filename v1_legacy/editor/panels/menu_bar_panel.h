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
    MenuBarPanel();
    virtual ~MenuBarPanel() = default;

    virtual void OnImGuiRender() override;
};
} // namespace CHEngine

#endif // CD_EDITOR_PANELS_MENU_BAR_PANEL_H
