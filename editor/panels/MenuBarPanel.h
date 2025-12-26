#ifndef MENU_BAR_PANEL_H
#define MENU_BAR_PANEL_H

#include <functional>

namespace CHEngine
{
struct PanelVisibility
{
    bool Hierarchy = true;
    bool Inspector = true;
    bool Viewport = true;
    bool AssetBrowser = true;
    bool Console = true;
};

struct MenuBarCallbacks
{
    std::function<void()> OnNew;
    std::function<void()> OnOpen;
    std::function<void()> OnSave;
    std::function<void()> OnSaveAs;
    std::function<void()> OnExit;

    std::function<void()> OnUndo;
    std::function<void()> OnRedo;
    std::function<bool()> CanUndo;
    std::function<bool()> CanRedo;

    std::function<void(const char *)> TogglePanel;
    std::function<void()> OnShowProjectSettings;
    std::function<void()> OnAbout;
};

class MenuBarPanel
{
public:
    MenuBarPanel() = default;

    void OnImGuiRender(const PanelVisibility &visibility, const MenuBarCallbacks &callbacks);
};
} // namespace CHEngine

#endif // MENU_BAR_PANEL_H
