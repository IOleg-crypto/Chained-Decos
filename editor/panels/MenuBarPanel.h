#ifndef MENU_BAR_PANEL_H
#define MENU_BAR_PANEL_H

#include <functional>

namespace CHEngine
{
/**
 * @brief Visibility states for editor panels
 */
struct PanelVisibility
{
public:
    bool Hierarchy = true;
    bool Inspector = true;
    bool Viewport = true;
    bool AssetBrowser = true;
    bool Console = true;
};

/**
 * @brief Callbacks for menu bar actions
 */
struct MenuBarCallbacks
{
public:
    // --- Project Actions ---
    std::function<void()> OnNewProject;
    std::function<void()> OnOpenProject;
    std::function<void()> OnCloseProject;

    // --- Scene Actions ---
    std::function<void()> OnNew;
    std::function<void()> OnOpen;
    std::function<void()> OnSave;
    std::function<void()> OnSaveAs;
    std::function<void()> OnPlayInRuntime;
    std::function<void()> OnExit;

    // --- Edit Actions ---
    std::function<void()> OnUndo;
    std::function<void()> OnRedo;
    std::function<bool()> CanUndo;
    std::function<bool()> CanRedo;

    // --- Window Actions ---
    std::function<void(const char *)> TogglePanel;
    std::function<void()> OnShowProjectSettings;
    std::function<void()> OnAbout;
};

/**
 * @brief Top menu bar panel
 */
class MenuBarPanel
{
public:
    MenuBarPanel() = default;
    ~MenuBarPanel() = default;

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender(const PanelVisibility &visibility, const MenuBarCallbacks &callbacks);
};
} // namespace CHEngine

#endif // MENU_BAR_PANEL_H
