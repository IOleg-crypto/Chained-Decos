#ifndef MENU_PRESENTER_H
#define MENU_PRESENTER_H

#include "MenuConstants.h"
#include <functional>
#include <imgui.h>
#include <string>


// Forward declarations
enum class MenuAction : uint8_t;
enum class MenuState : uint8_t;

// Callback type for action handling
using ActionCallback = std::function<void(MenuAction)>;

class MenuPresenter
{
public:
    MenuPresenter() = default;
    ~MenuPresenter() = default;

    // Setup
    void SetActionCallback(ActionCallback callback);
    void SetupStyle();

    // Rendering helpers
    bool RenderActionButton(const char *label, MenuAction action,
                            const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;
    void RenderMenuHint(const char *text) const;

    // Screen renderers
    void RenderMainMenu(bool gameInProgress, bool addResumeButton);
    void RenderOptionsMenu();
    void RenderGameModeMenu();
    void RenderCreditsScreen();
    void RenderModsScreen();
    void RenderConfirmExitDialog();

    // Utility
    static const char *GetStateTitle(MenuState state);

private:
    ActionCallback m_actionCallback;
    ImGuiStyle m_customStyle{};
};

#endif // MENU_PRESENTER_H




