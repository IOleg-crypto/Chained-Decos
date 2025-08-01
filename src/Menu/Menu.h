#ifndef MENU_H
#define MENU_H

#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

// # ----------------------------------------------------------------------------
// # Menu - handles rendering and logic for main menu
// # ----------------------------------------------------------------------------

enum class MenuAction {
    None,
    StartGame,
    OpenOptions,
    ExitGame
};

class Menu {
private:
    int selected = 0;
    MenuAction action = MenuAction::None;

public:
    void Update();
    void Render() const;
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();
};

#endif
