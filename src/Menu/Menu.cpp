//
// Created by I#Oleg
//

#include "Menu.h"
#include "raylib.h"

#include <iostream>

void Menu::Update() {
    const Vector2 mousePosition = GetMousePosition();
    const int startY = 300;
    const int buttonWidth = 160;
    const int buttonHeight = 40;

    if (IsKeyPressed(KEY_DOWN)) {
        selected = (selected + 1) % 3;
    }
    if (IsKeyPressed(KEY_UP)) {
        selected = (selected + 2) % 3;
    }


    for (int i = 0; i < 3; i++) {
        int x = GetScreenWidth() / 2 - buttonWidth / 2;
        int y = startY + i * 50;
        Rectangle btnRect = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(buttonWidth), static_cast<float>(buttonHeight) };
        if (CheckCollisionPointRec(mousePosition, btnRect)) {
            selected = i;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                switch (selected) {
                    case 0: action = MenuAction::StartGame; break;
                    case 1: action = MenuAction::OpenOptions; break;
                    case 2: action = MenuAction::ExitGame; break;
                    default: ;
                }
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        switch (selected) {
            case 0: action = MenuAction::StartGame; break;
            case 1: action = MenuAction::OpenOptions; break;
            case 2: action = MenuAction::ExitGame; break;
            default: ;
        }
    }
}

void Menu::Render() const {
    Vector2 mousePosition = GetMousePosition();
    const char* options[] = { "Start Game", "Options", "Quit" };
    int buttonWidth = 160;
    int buttonHeight = 40;
    int startY = 300;

    DrawText("Chained Decos", GetScreenWidth() / 2 - 120, 100 + 1 * 50, 45, WHITE);

    for (int i = 0; i < std::size(options); i++) {
        const int x = GetScreenWidth() / 2 - buttonWidth / 2;
        const int y = startY + i * 50;
        const Rectangle btnRect = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(buttonWidth), static_cast<float>(buttonHeight) };

        Color color = (i == selected) ? YELLOW : LIGHTGRAY;
        if (CheckCollisionPointRec(mousePosition, btnRect)) {
            color = ORANGE;
        }

        DrawRectangleRec(btnRect, Fade(color, 0.2f));
        DrawText(options[i], x + 10, y + 5, 30, color);
    }
}

MenuAction Menu::GetAction() const {
    return action;
}

void Menu::ResetAction() {
    action = MenuAction::None;
}
