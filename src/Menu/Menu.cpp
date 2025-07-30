//
// Created by I#Oleg
//

#include <iostream>
#include <Menu/Menu.h>

void Menu::Update() {
    if (IsKeyPressed(KEY_DOWN)) selected = (selected + 1) % 3;
    if (IsKeyPressed(KEY_UP))   selected = (selected + 2) % 3;

    if (IsKeyPressed(KEY_ENTER)) {
        switch (selected) {
            case 0:
                action = MenuAction::StartGame; break;
            case 1:
                action = MenuAction::OpenOptions; break;
            case 2:
                action = MenuAction::ExitGame; break;
            default:
                break;
        }
    }
}

void Menu::Render() const {
    DrawText("Chained Decos", GetScreenWidth() / 2 - 120, 100 + 1 * 50, 45, WHITE);
    const char* options[] = { "Start Game", "Options", "Quit" };

    for (int i = 0; i < std::size(options); i++) {
        const Color color = (i == selected) ? YELLOW : LIGHTGRAY;
        DrawText(options[i], GetScreenWidth() / 2 - 80, 300 + i * 50, 30, color);
    }
}

MenuAction Menu::GetAction() const {
    return action;
}

void Menu::ResetAction() {
    action = MenuAction::None;
}
