//
// Created by I#Oleg
//

#include <Engine/Engine.h>
#include <Menu/Menu.h>
#include <raylib.h>

#include <iostream>

Menu::Menu()
{
    m_mainMenu = {{{.label = "Start Game", .action = MenuAction::StartGame},
                   {.label = "Options", .action = MenuAction::OpenOptions},
                   {.label = "Credits", .action = MenuAction::OpenCredits},
                   {.label = "Quit", .action = MenuAction::ExitGame}}};

    m_optionsMenu = {{{.label = "Video", .action = MenuAction::OpenVideoMode},
                      {.label = "Audio", .action = MenuAction::OpenAudio},
                      {.label = "Controls", .action = MenuAction::OpenControls},
                      {.label = "Back", .action = MenuAction::BackToMainMenu}}};

    m_SetGameMode = {{{.label = "Singleplayer", .action = MenuAction::SinglePlayer},
                      {.label = "Multiplayer", .action = MenuAction::MultiPlayer},
                      {.label = "Back", .action = MenuAction::BackToMainMenu}}};

    m_videoMenu = {{.label = "Resolution", .action = MenuAction::None},
                   {.label = "Fullscreen", .action = MenuAction::None},
                   {.label = "VSync", .action = MenuAction::None},
                   {.label = "Back", .action = MenuAction::BackToMainMenu}};

    m_audioMenu = {{.label = "Master Volume", .action = MenuAction::None},
                   {.label = "Music Volume", .action = MenuAction::None},
                   {.label = "SFX Volume", .action = MenuAction::None},
                   {.label = "Back", .action = MenuAction::BackToMainMenu}};

    m_controlsMenu = {{.label = "Rebind Keys", .action = MenuAction::None},
                      {.label = "Invert Y Axis", .action = MenuAction::None},
                      {.label = "Back", .action = MenuAction::BackToMainMenu}};
}
void Menu::Update()
{
    switch (m_state)
    {
    case MenuState::Main:
        m_currentMenu = m_mainMenu;
        break;
    case MenuState::Options:
        m_currentMenu = m_optionsMenu;
        break;
    case MenuState::GameMode:
        m_currentMenu = m_SetGameMode;
        break;
    case MenuState::Video:
        m_currentMenu = m_videoMenu;
        break;
    case MenuState::Audio:
        m_currentMenu = m_audioMenu;
        break;
    case MenuState::Controls:
        m_currentMenu = m_controlsMenu;
        break;
    default:
        break;
    }

    Vector2 mousePos = GetMousePosition();

    constexpr int kBtnW = 200;
    constexpr int kBtnH = 50;
    constexpr int kStartY = 300;
    constexpr int kSpacing = 70;

    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_currentMenu.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_currentMenu.size() - 1) % m_currentMenu.size();
    if (IsKeyPressed(KEY_TAB))
        m_selected = (m_selected + 1) % m_currentMenu.size();
    if ((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_TAB))
        m_selected = (m_selected + m_currentMenu.size() - 1) % m_currentMenu.size();
    if (IsKeyPressed(KEY_ENTER))
        m_action = m_currentMenu[m_selected].action;
    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_state == MenuState::Options || m_state == MenuState::GameMode)
            m_state = MenuState::Main;
        else if (m_state == MenuState::Main)
            m_action = MenuAction::ExitGame;
    }

    for (size_t i = 0; i < m_currentMenu.size(); ++i)
    {
        int x = GetScreenWidth() / 2 - kBtnW / 2;
        int y = kStartY + int(i) * kSpacing;
        Rectangle btnRect = {(float)x, (float)y, (float)kBtnW, (float)kBtnH};

        if (CheckCollisionPointRec(mousePos, btnRect))
        {
            m_selected = (int)i;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                m_action = m_currentMenu[i].action;
        }
    }

    switch (m_action)
    {
    case MenuAction::StartGame:
        m_state = MenuState::GameMode;
        break;
    case MenuAction::SinglePlayer:
    case MenuAction::MultiPlayer:
        std::cout << "Selected mode: "
                  << ((m_action == MenuAction::SinglePlayer) ? "Singleplayer" : "Multiplayer")
                  << '\n';
        break;
    case MenuAction::OpenOptions:
        m_state = MenuState::Options;
        break;
    case MenuAction::OpenVideoMode:
        m_state = MenuState::Video;
        break;
    case MenuAction::OpenAudio:
        m_state = MenuState::Audio;
        break;
    case MenuAction::OpenControls:
        m_state = MenuState::Controls;
        break;
    case MenuAction::BackToMainMenu:
        if (m_state == MenuState::Video || m_state == MenuState::Audio ||
            m_state == MenuState::Controls)
            m_state = MenuState::Options;
        else
            m_state = MenuState::Main;
        break;
    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Exit requested");
        ResetAction();
        if (m_engine)
        {
            m_engine->RequestExit();
        }
        break;
    default:
        break;
    }
}

float Menu::Lerp(float a, float b, float t) const { return a + (b - a) * t; }

void Menu::Render() const
{
    const auto &currentMenu = m_currentMenu;
    Vector2 mousePos = GetMousePosition();

    constexpr int kBtnW = 200;
    constexpr int kBtnH = 50;
    constexpr int kStartY = 300;
    constexpr int kSpacing = 70;

    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = (float)i / (float)GetScreenHeight();
        Color c = {(unsigned char)(15 + 25 * t), (unsigned char)(15 + 30 * t),
                   (unsigned char)(40 + 90 * t), 255};
        DrawLine(0, i, GetScreenWidth(), i, c);
    }
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));

    const char *title;
    if (MenuState::Main == m_state)
    {
        title = "Chained Decos";
    }
    else
    {
        title = currentMenu[m_selected].label;
    }
    // DrawText(title, 100,
    int tw = MeasureText(title, 56);
    int tx = GetScreenWidth() / 2 - tw / 2;
    DrawText(title, tx + 4, 104, 56, Fade(BLACK, 0.75f));
    DrawText(title, tx, 100, 56, RAYWHITE);

    if (m_buttonScales.size() != currentMenu.size())
        ((Menu *)this)->m_buttonScales.assign(currentMenu.size(), 1.0f);

    for (size_t i = 0; i < currentMenu.size(); ++i)
    {
        // Section headers for better organization
        if (m_state == MenuState::Options)
        {
            if (i == currentMenu.size() - 1)
            {
                // Divider before 'Back'
                DrawLine(GetScreenWidth() / 2 - kBtnW / 2, kStartY + int(i) * kSpacing - 15,
                         GetScreenWidth() / 2 + kBtnW / 2, kStartY + int(i) * kSpacing - 15,
                         Fade(RAYWHITE, 0.2f));
            }
        }
        if (m_state == MenuState::Main && i == currentMenu.size() - 1)
        {
            // Divider before 'Quit'
            DrawLine(GetScreenWidth() / 2 - kBtnW / 2, kStartY + int(i) * kSpacing - 15,
                     GetScreenWidth() / 2 + kBtnW / 2, kStartY + int(i) * kSpacing - 15,
                     Fade(ORANGE, 0.2f));
        }

        int baseX = GetScreenWidth() / 2 - kBtnW / 2;
        int baseY = kStartY + int(i) * kSpacing;
        Rectangle baseRect = {(float)baseX, (float)baseY, (float)kBtnW, (float)kBtnH};

        bool hovered = CheckCollisionPointRec(mousePos, baseRect);
        bool selected = ((int)i == m_selected);

        float targetScale = (hovered || selected) ? 1.10f : 1.00f;
        float s = Lerp(m_buttonScales[i], targetScale, 0.15f);
        ((Menu *)this)->m_buttonScales[i] = s;

        int w = (int)(kBtnW * s);
        int h = (int)(kBtnH * s);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = kStartY + int(i) * kSpacing - (h - kBtnH) / 2;
        Rectangle btnRect = {(float)x, (float)y, (float)w, (float)h};

        DrawRectangle(x + 5, y + 6, w, h, Fade(BLACK, 0.35f));

        // Gradient
        Color topColor, bottomColor, borderColor;
        if (selected)
        {
            topColor = {.r = 255, .g = 240, .b = 200, .a = 255};
            bottomColor = {.r = 220, .g = 175, .b = 90, .a = 255};
            borderColor = ORANGE;
        }
        else if (hovered)
        {
            topColor = {.r = 245, .g = 220, .b = 165, .a = 255};
            bottomColor = {.r = 205, .g = 150, .b = 85, .a = 255};
            borderColor = {.r = 50, .g = 35, .b = 25, .a = 255};
        }
        else
        {
            topColor = {.r = 200, .g = 200, .b = 200, .a = 255};
            bottomColor = {.r = 130, .g = 130, .b = 130, .a = 255};
            borderColor = {.r = 35, .g = 35, .b = 35, .a = 255};
        }

        for (int j = 0; j < h; ++j)
        {
            float t = (float)j / (float)h;
            Color c = {(unsigned char)(topColor.r + t * (bottomColor.r - topColor.r)),
                       (unsigned char)(topColor.g + t * (bottomColor.g - topColor.g)),
                       (unsigned char)(topColor.b + t * (bottomColor.b - topColor.b)), 255};
            DrawLine(x, y + j, x + w, y + j, c);
        }

        int glossH = (int)(h * 0.40f);
        DrawRectangle(x + 1, y + 1, w - 2, glossH, Fade(WHITE, 0.06f));

        // Thicker border and glow for selected
        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 4, borderColor);
            DrawRectangleLinesEx(
                (Rectangle){btnRect.x - 4, btnRect.y - 4, btnRect.width + 8, btnRect.height + 8}, 2,
                Fade(borderColor, 0.3f));
        }
        else
        {
            DrawRectangleLinesEx(btnRect, 2, borderColor);
        }

        int fontSize = 28;
        int textW = MeasureText(currentMenu[i].label, fontSize);
        int textX = x + w / 2 - textW / 2;
        int textY = y + h / 2 - fontSize / 2;

        DrawText(currentMenu[i].label, textX + 2, textY + 2, fontSize, Fade(BLACK, 0.7f));
        DrawText(currentMenu[i].label, textX, textY, fontSize, RAYWHITE);
    }

    // Draw footer with keyboard/mouse hints
    const char *footer = "[Enter] Select   [Esc] Back   [↑/↓] Navigate   [Mouse] Click";
    int fw = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

MenuAction Menu::GetAction() const { return m_action; }

void Menu::GetEngine(Engine *engine) { m_engine = engine; }

void Menu::ResetAction() { m_action = MenuAction::None; }

void Menu::RenderSettingsMenu() const {}