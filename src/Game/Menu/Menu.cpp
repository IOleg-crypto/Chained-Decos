#include "Menu.h"
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <string>
#include <vector>

Menu::Menu()
{
    m_mainMenu = {{{"Start Game", MenuAction::StartGame},
                   {"Options", MenuAction::OpenOptions},
                   {"Mods", MenuAction::OpenMods},
                   {"Credits", MenuAction::OpenCredits},
                   {"Quit", MenuAction::ExitGame}}};

    m_optionsMenu = {{{"Video", MenuAction::OpenVideoMode},
                      {"Audio", MenuAction::OpenAudio},
                      {"Controls", MenuAction::OpenControls},
                      {"Back", MenuAction::BackToMainMenu}}};

    m_SetGameMode = {{{"Singleplayer", MenuAction::SinglePlayer},
                      {"Multiplayer", MenuAction::MultiPlayer},
                      {"Back", MenuAction::BackToMainMenu}}};

    m_videoOptions = {
        {"Resolution", {"800x600", "1280x720", "1360x768", "1920x1080", "2560x1440"}, 0},
        {"Aspect Ratio", {"16:9", "4:3", "21:9"}, 0},
        {"Display Mode", {"Windowed", "Fullscreen", "Borderless"}, 0},
        {"VSync", {"Off", "On"}, 1},
        {"Back", {}, 0}};

    m_audioMenu = {{{"Master Volume", MenuAction::None},
                    {"Music Volume", MenuAction::None},
                    {"SFX Volume", MenuAction::None},
                    {"Back", MenuAction::BackToMainMenu}}};

    m_controlsMenu = {{{"Rebind Keys", MenuAction::None},
                       {"Invert Y Axis", MenuAction::None},
                       {"Back", MenuAction::BackToMainMenu}}};

    m_currentMenu = m_mainMenu;
    m_buttonScales.assign(m_currentMenu.size(), 1.3f);
}

Menu::~Menu() = default;

float Menu::Lerp(const float a, const float b, const float t) const { return a + (b - a) * t; }

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
    case MenuState::Audio:
        m_currentMenu = m_audioMenu;
        break;
    case MenuState::Controls:
        m_currentMenu = m_controlsMenu;
        break;
    case MenuState::Video:
    case MenuState::Credits:
    case MenuState::Mods:
        break;
    case MenuState::ConfirmExit:
        break;
    default:
        break;
    }

    if (m_state == MenuState::Video)
        HandleVideoNavigation();
    else if (m_state == MenuState::Credits || m_state == MenuState::Mods)
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ONE))
        {
            m_state = MenuState::Main;
            m_selected = 0;
        }
    }
    else if (m_state == MenuState::ConfirmExit)
    {
        HandleConfirmExit();
    }
    ExecuteAction();
}

void Menu::HandleKeyboardNavigation()
{
    if (m_state == MenuState::Credits || m_state == MenuState::Mods)
        return;

    if (m_state != MenuState::Video)
    {
        if (IsKeyPressed(KEY_DOWN))
            m_selected = (m_selected + 1) % m_currentMenu.size();
        if (IsKeyPressed(KEY_UP))
            m_selected = (m_selected + m_currentMenu.size() - 1) % m_currentMenu.size();
    }

    if (IsKeyPressed(KEY_ENTER))
        m_action = m_currentMenu[m_selected].action;

    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_state == MenuState::Options || m_state == MenuState::GameMode)
            m_state = MenuState::Main;
        else if (m_state == MenuState::Main)
            m_action = MenuAction::ExitGame;
    }
}

void Menu::HandleVideoNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_videoOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_videoOptions.size() - 1) % m_videoOptions.size();

    if (!m_videoOptions[m_selected].values.empty())
    {
        if (IsKeyPressed(KEY_RIGHT))
            m_videoOptions[m_selected].selectedIndex =
                (m_videoOptions[m_selected].selectedIndex + 1) %
                m_videoOptions[m_selected].values.size();
        if (IsKeyPressed(KEY_LEFT))
            m_videoOptions[m_selected].selectedIndex =
                (m_videoOptions[m_selected].selectedIndex +
                 m_videoOptions[m_selected].values.size() - 1) %
                m_videoOptions[m_selected].values.size();
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        auto &opt = m_videoOptions[m_selected];
        if (opt.label == "Back")
            m_state = MenuState::Options;
        if (opt.label == "Resolution" && !opt.values.empty())
        {
            int width = 0, height = 0;
            sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &width, &height);
            SetWindowSize(width, height);
        }
        else if (opt.label == "Display Mode" && !opt.values.empty())
        {
            std::string mode = opt.values[opt.selectedIndex];
            if (mode == "Fullscreen")
                SetWindowState(FLAG_FULLSCREEN_MODE);
            else if (mode == "Windowed")
            {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
            }
            else if (mode == "Borderless")
                SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_FULLSCREEN_MODE);
        }
        else if (opt.label == "VSync" && !opt.values.empty())
        {
            const std::string &vsync = opt.values[opt.selectedIndex];
            SetTargetFPS(vsync == "On" ? GetMonitorRefreshRate(GetCurrentMonitor()) : 0);
        }
    }
}

void Menu::HandleMouseSelection()
{
    if (m_state == MenuState::Credits || m_state == MenuState::Mods)
        return;

    Vector2 mousePos = GetMousePosition();
    constexpr int kBtnW = 200, kBtnH = 50, kStartY = 300, kSpacing = 70;

    bool clickedThisFrame = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    for (size_t i = 0; i < m_currentMenu.size(); ++i)
    {
        int x = GetScreenWidth() / 2 - kBtnW / 2;
        int y = kStartY + static_cast<int>(i) * kSpacing;
        const Rectangle btnRect = {static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(kBtnW), static_cast<float>(kBtnH)};

        if (CheckCollisionPointRec(mousePos, btnRect))
        {
            m_selected = static_cast<int>(i);
            if (clickedThisFrame)
            {
                m_action = m_currentMenu[i].action;
                break;
            }
        }
    }
}

void Menu::ExecuteAction()
{
    if (m_action == MenuAction::None)
        return;

    switch (m_action)
    {
    case MenuAction::StartGame:
        m_state = MenuState::GameMode;
        // m_selected не скидаємо
        ResetAction();
        break;

    case MenuAction::SinglePlayer:
    case MenuAction::MultiPlayer:
        std::cout << "Selected mode: "
                  << (m_action == MenuAction::SinglePlayer ? "Singleplayer" : "Multiplayer")
                  << '\n';

        break;

    case MenuAction::OpenOptions:
        m_state = MenuState::Options;
        ResetAction();
        break;

    case MenuAction::OpenVideoMode:
        m_state = MenuState::Video;
        ResetAction();
        break;

    case MenuAction::OpenAudio:
        m_state = MenuState::Audio;
        ResetAction();
        break;

    case MenuAction::OpenControls:
        m_state = MenuState::Controls;
        ResetAction();
        break;

    case MenuAction::OpenCredits:
        m_state = MenuState::Credits;
        ResetAction();
        break;

    case MenuAction::OpenMods:
        m_state = MenuState::Mods;
        ResetAction();
        break;

    case MenuAction::BackToMainMenu:
        if (m_state == MenuState::Video || m_state == MenuState::Audio ||
            m_state == MenuState::Controls)
            m_state = MenuState::Options;
        else
            m_state = MenuState::Main;
        ResetAction();
        break;

    case MenuAction::ExitGame:
        if (m_state == MenuState::ConfirmExit)
        {
            TraceLog(LOG_INFO, "Exit requested");
            // CallExitToEngine(); // Видалено
        }
        else
        {
            m_state = MenuState::ConfirmExit;
        }
        ResetAction();
        break;

    default:
        ResetAction();
        break;
    }
}

void Menu::ResetAction() { m_action = MenuAction::None; }
MenuAction Menu::GetAction() const { return m_action; }

void Menu::Render()
{
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(GetScreenHeight());
        DrawLine(0, i, GetScreenWidth(), i,
                 Color{static_cast<unsigned char>(15 + 25 * t),
                       static_cast<unsigned char>(15 + 30 * t),
                       static_cast<unsigned char>(40 + 90 * t), 255});
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));

    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        RenderMenu();
        break;
    case MenuState::Video:
        RenderSettingsMenu();
        break;
    case MenuState::Credits:
        RenderCredits();
        break;
    case MenuState::Mods:
        RenderMods();
        break;
    case MenuState::ConfirmExit:
        RenderConfirmExit();
        break;
    default:
        break;
    }
}

void Menu::RenderMenu() const
{
    const auto &currentMenu = m_currentMenu;
    const Vector2 mousePos = GetMousePosition();
    constexpr int kBtnW = 200, kBtnH = 50, kStartY = 300, kSpacing = 70;

    const char *title = (m_state == MenuState::Main)
                            ? "Chained Decos"
                            : currentMenu[static_cast<size_t>(m_selected)].label;
    int tw = MeasureText(title, 56);
    DrawText(title, GetScreenWidth() / 2 - tw / 2 + 4, 104, 56, Fade(BLACK, 0.75f));
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 100, 56, RAYWHITE);

    if (m_buttonScales.size() != currentMenu.size())
        m_buttonScales.assign(currentMenu.size(), 1.0f);

    for (size_t i = 0; i < currentMenu.size(); ++i)
    {
        int baseX = GetScreenWidth() / 2 - kBtnW / 2;
        int baseY = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle baseRect = {static_cast<float>(baseX), static_cast<float>(baseY),
                              static_cast<float>(kBtnW), static_cast<float>(kBtnH)};
        bool hovered = CheckCollisionPointRec(mousePos, baseRect);
        bool selected = (static_cast<int>(i) == m_selected);
        float targetScale = (hovered || selected) ? 1.10f : 1.0f;
        m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.15f);

        int w = static_cast<int>(kBtnW * m_buttonScales[i]);
        int h = static_cast<int>(kBtnH * m_buttonScales[i]);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = baseY - (h - kBtnH) / 2;
        Rectangle btnRect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                             static_cast<float>(h)};
        DrawRectangle(x + 5, y + 6, w, h, Fade(BLACK, 0.35f));

        Color topColor, bottomColor, borderColor;
        if (selected)
        {
            topColor = {255, 240, 200, 255};
            bottomColor = {220, 175, 90, 255};
            borderColor = ORANGE;
        }
        else if (hovered)
        {
            topColor = {245, 220, 165, 255};
            bottomColor = {205, 150, 85, 255};
            borderColor = {50, 35, 25, 255};
        }
        else
        {
            topColor = {200, 200, 200, 255};
            bottomColor = {130, 130, 130, 255};
            borderColor = {35, 35, 35, 255};
        }

        for (int j = 0; j < h; ++j)
        {
            float t = static_cast<float>(j) / static_cast<float>(h);
            Color c = {static_cast<unsigned char>(topColor.r + t * (bottomColor.r - topColor.r)),
                       static_cast<unsigned char>(topColor.g + t * (bottomColor.g - topColor.g)),
                       static_cast<unsigned char>(topColor.b + t * (bottomColor.b - topColor.b)),
                       255};
            DrawLine(x, y + j, x + w, y + j, c);
        }

        DrawRectangle(x + 1, y + 1, w - 2, static_cast<int>(h * 0.4f), Fade(WHITE, 0.06f));
        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 4, borderColor);
            DrawRectangleLinesEx(
                Rectangle{btnRect.x - 4, btnRect.y - 4, btnRect.width + 8, btnRect.height + 8}, 2,
                Fade(borderColor, 0.3f));
        }
        else
            DrawRectangleLinesEx(btnRect, 2, borderColor);

        int textW = MeasureText(currentMenu[i].label, 28);
        int textX = x + w / 2 - textW / 2;
        int textY = y + h / 2 - 28 / 2;
        DrawText(currentMenu[i].label, textX + 2, textY + 2, 28, Fade(BLACK, 0.7f));
        DrawText(currentMenu[i].label, textX, textY, 28, RAYWHITE);
    }

    const char *footer = "[Enter] Select   [Esc] Back   [↑/↓] Navigate   [Mouse] Click";
    int fw = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderSettingsMenu()
{
    int startY = 150, spacing = 50, fontSize = 28;
    DrawText("Video Settings", 80, 50, 40, ORANGE);
    for (size_t i = 0; i < m_videoOptions.size(); i++)
    {
        auto &[label, values, selectedIndex] = m_videoOptions[i];
        int y = startY + static_cast<int>(i) * spacing;
        DrawText(label.c_str(), 80, y, fontSize,
                 static_cast<int>(i) == m_selected ? ORANGE : RAYWHITE);

        if (!values.empty())
        {
            std::string value = "< " + values[selectedIndex] + " >";
            int textWidth = MeasureText(value.c_str(), fontSize);
            DrawText(value.c_str(), GetScreenWidth() - textWidth - 80, y, fontSize,
                     static_cast<int>(i) == m_selected ? GOLD : YELLOW);
        }
    }
    std::string footer = "[Enter] Apply/Select [←/→] Change [↑/↓] Navigate [Esc] Back";
    int fw = MeasureText(footer.c_str(), 24);
    DrawText(footer.c_str(), GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderCredits()
{
    const char *title = "Credits";
    int tw = MeasureText(title, 48);
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 80, 48, ORANGE);

    int y = 160, fs = 26;
    DrawText("Developer: I#Oleg", 80, y, fs, RAYWHITE);
    y += 36;
    DrawText("Engine: raylib + rlImGui", 80, y, fs, RAYWHITE);
    y += 36;
    DrawText("UI: Custom styled buttons", 80, y, fs, RAYWHITE);

    const char *footer = "[Esc] Back";
    int fw2 = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw2 / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderMods()
{
    const char *title = "Mods";
    int tw = MeasureText(title, 48);
    DrawText(title, GetScreenWidth() / 2 - tw / 2, 80, 48, ORANGE);

    int fs = 24;
    DrawText("No mods detected.", 80, 160, fs, RAYWHITE);
    DrawText("Place your mods in the 'resources/mods' folder.", 80, 200, fs, RAYWHITE);

    const char *footer = "[Esc] Back";
    int fw2 = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw2 / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::HandleConfirmExit()
{
    // Підтвердження виходу: Y/N або Enter/Esc
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER))
    {
        m_action = MenuAction::ExitGame;
    }
    else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::Main;
        m_selected = 0;
    }
}

void Menu::RenderConfirmExit() const
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));
    const char *msg = "Вийти з гри?";
    int tw = MeasureText(msg, 48);
    DrawText(msg, GetScreenWidth() / 2 - tw / 2, GetScreenHeight() / 2 - 80, 48, ORANGE);
    const char *yes = "[Y/Enter] Так";
    const char *no = "[N/Esc] Ні";
    int yw = MeasureText(yes, 32);
    int nw = MeasureText(no, 32);
    DrawText(yes, GetScreenWidth() / 2 - yw - 20, GetScreenHeight() / 2, 32, RAYWHITE);
    DrawText(no, GetScreenWidth() / 2 + 20, GetScreenHeight() / 2, 32, RAYWHITE);
}
