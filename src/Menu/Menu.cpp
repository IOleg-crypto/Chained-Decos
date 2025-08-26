#include <Engine/Engine.h>
#include <Menu/Menu.h>
#include <raylib.h>

#include <iostream>

Menu::Menu(): m_engine(nullptr) {
    m_mainMenu = {
        {
            {"Start Game", MenuAction::StartGame},
            {"Options", MenuAction::OpenOptions},
            {"Credits", MenuAction::OpenCredits},
            {"Quit", MenuAction::ExitGame}
        }
    };

    m_optionsMenu = {
        {
            {"Video", MenuAction::OpenVideoMode},
            {"Audio", MenuAction::OpenAudio},
            {"Controls", MenuAction::OpenControls},
            {"Back", MenuAction::BackToMainMenu}
        }
    };

    m_SetGameMode = {
        {
            {"Singleplayer", MenuAction::SinglePlayer},
            {"Multiplayer", MenuAction::MultiPlayer},
            {"Back", MenuAction::BackToMainMenu}
        }
    };

    m_videoOptions = {
        {"Resolution", {"800x600" , "1280x720" , "1360x768" , "1920x1080", "2560x1440"}, 0},
        {"Aspect Ratio", {"16:9", "4:3", "21:9"}, 0},
        {"Display Mode", {"Windowed", "Fullscreen", "Borderless"}, 0},
        {"VSync", {"Off", "On"}, 1},
        {"Back", {}, 0}
    };

    m_audioMenu = {
        {
            {"Master Volume", MenuAction::None},
            {"Music Volume", MenuAction::None},
            {"SFX Volume", MenuAction::None},
            {"Back", MenuAction::BackToMainMenu}
        }
    };

    m_controlsMenu = {
        {
            {"Rebind Keys", MenuAction::None},
            {"Invert Y Axis", MenuAction::None},
            {"Back", MenuAction::BackToMainMenu}
        }
    };

    m_currentMenu = m_mainMenu;
    m_buttonScales.assign(m_currentMenu.size(), 1.0f);
}

float Menu::Lerp(const float a, const float b, const float t) const {
    return a + (b - a) * t;
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
        HandleVideoNavigation();
        break;
    case MenuState::Audio:
        m_currentMenu = m_audioMenu;
        break;
    case MenuState::Controls:
        m_currentMenu = m_controlsMenu;
        break;
    default: ;
    }

    if (IsKeyPressed(KEY_ENTER) && m_videoOptions[m_selected].label == "Back")
    {
        m_state = MenuState::Options;
    }

    HandleKeyboardNavigation();
    HandleMouseSelection();

    ExecuteAction();
}

void Menu::HandleVideoNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_videoOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_videoOptions.size() - 1) % m_videoOptions.size();

    if (IsKeyPressed(KEY_RIGHT))
        m_videoOptions[m_selected].selectedIndex = (m_videoOptions[m_selected].selectedIndex + 1) %
                                                   m_videoOptions[m_selected].values.size();

    if (IsKeyPressed(KEY_LEFT))
        m_videoOptions[m_selected].selectedIndex = (m_videoOptions[m_selected].selectedIndex +
                                                    m_videoOptions[m_selected].values.size() - 1) %
                                                   m_videoOptions[m_selected].values.size();
}

void Menu::HandleKeyboardNavigation()
{
    if (m_state != MenuState::Video)
    {
        if (IsKeyPressed(KEY_DOWN))
            m_selected = (m_selected + 1) % m_currentMenu.size();
        if (IsKeyPressed(KEY_UP))
            m_selected = (m_selected + m_currentMenu.size() - 1) % m_currentMenu.size();
    }

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
}

void Menu::HandleMouseSelection()
{
    Vector2 mousePos = GetMousePosition();
    constexpr int kBtnW = 200;
    constexpr int kBtnH = 50;
    constexpr int kStartY = 300;
    constexpr int kSpacing = 70;

    for (size_t i = 0; i < m_currentMenu.size(); ++i)
    {
        int x = GetScreenWidth() / 2 - kBtnW / 2;
        int y = kStartY + static_cast<int>(i) * kSpacing;
        const Rectangle btnRect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(kBtnW), static_cast<float>(kBtnH)};

        if (CheckCollisionPointRec(mousePos, btnRect))
        {
            m_selected = static_cast<int>(i);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                m_action = m_currentMenu[i].action;
        }
    }
}

void Menu::ExecuteAction()
{
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
            m_engine->RequestExit();
        break;
    default:
        break;
    }
}

void Menu::GetEngine(Engine *engine) { m_engine = engine; }

void Menu::ResetAction() { m_action = MenuAction::None; }

MenuAction Menu::GetAction() const { return m_action; }

void Menu::Render(){
    for (int i = 0; i < GetScreenHeight(); ++i)
    {
        auto t = static_cast<float>(i) / GetScreenHeight();
        Color color = {static_cast<unsigned char>(15 + 25 * t), static_cast<unsigned char>(15 + 30 * t),
                   static_cast<unsigned char>(40 + 90 * t), 255};
        DrawLine(0, i, GetScreenWidth(), i, color);
    }
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));

        switch (m_state) {
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

            default:
                break;
        }
}

void Menu::RenderMenu() {
    const auto &currentMenu = m_currentMenu;
    Vector2 mousePos = GetMousePosition();

    constexpr int kBtnW = 200;
    constexpr int kBtnH = 50;
    constexpr int kStartY = 300;
    constexpr int kSpacing = 70;

    const char *title =
        (m_state == MenuState::Main) ? "Chained Decos" : currentMenu[m_selected].label;
    int tw = MeasureText(title, 56);
    int tx = GetScreenWidth() / 2 - tw / 2;
    DrawText(title, tx + 4, 104, 56, Fade(BLACK, 0.75f));
    DrawText(title, tx, 100, 56, RAYWHITE);

    if (m_buttonScales.size() != currentMenu.size())
        m_buttonScales.assign(currentMenu.size(), 1.0f);

    for (size_t i = 0; i < currentMenu.size(); ++i)
    {
        int baseX = GetScreenWidth() / 2 - kBtnW / 2;
        int baseY = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle baseRect = {static_cast<float>(baseX), static_cast<float>(baseY), static_cast<float>(kBtnW), static_cast<float>(kBtnH)};

        bool hovered = CheckCollisionPointRec(mousePos, baseRect);
        bool selected = static_cast<int>(i) == m_selected;

        float targetScale = (hovered || selected) ? 1.10f : 1.00f;
        m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.15f);

        int w = static_cast<int>(kBtnW * m_buttonScales[i]);
        int h = static_cast<int>(kBtnH * m_buttonScales[i]);
        int x = GetScreenWidth() / 2 - w / 2;
        int y = kStartY + int(i) * kSpacing - (h - kBtnH) / 2;
        Rectangle btnRect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};

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
            float t = static_cast<float>(j) / h;
            Color c = {static_cast<unsigned char>(topColor.r + t * (bottomColor.r - topColor.r)),
                       static_cast<unsigned char>(topColor.g + t * (bottomColor.g - topColor.g)),
                       static_cast<unsigned char>(topColor.b + t * (bottomColor.b - topColor.b)), 255};
            DrawLine(x, y + j, x + w, y + j, c);
        }

        int glossH = static_cast<int>(h * 0.40f);
        DrawRectangle(x + 1, y + 1, w - 2, glossH, Fade(WHITE, 0.06f));

        if (selected)
        {
            DrawRectangleLinesEx(btnRect, 4, borderColor);
            DrawRectangleLinesEx(
                {btnRect.x - 4, btnRect.y - 4, btnRect.width + 8, btnRect.height + 8}, 2,
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

    // Footer
    const char *footer = "[Enter] Select   [Esc] Back   [↑/↓] Navigate   [Mouse] Click";
    int fw = MeasureText(footer, 24);
    DrawText(footer, GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}

void Menu::RenderSettingsMenu() {
    int startY = 150;
    int spacing = 50;
    int fontSize = 28;


    DrawText("Video Settings", 80, 50, 40, ORANGE);

    for (size_t i = 0; i < m_videoOptions.size(); i++)
    {
        auto &[label, values, selectedIndex] = m_videoOptions[i];
        int y = startY + i * spacing;


        Color labelCol = (i == m_selected) ? ORANGE : RAYWHITE;
        Color valueCol = (i == m_selected) ? GOLD : YELLOW;


        DrawText(label.c_str(), 80, y, fontSize, labelCol);

        if (!values.empty()) {
            std::string value = "< " + values[selectedIndex] + " >";
            int textWidth = MeasureText(value.c_str(), fontSize);
            DrawText(value.c_str(), GetScreenWidth() - textWidth - 80, y, fontSize, valueCol);
        }


        if (i == m_selected && IsKeyPressed(KEY_ENTER))
        {
            if (label == "Resolution" && !values.empty()) {
                std::string res = values[selectedIndex];
                int width = GetScreenWidth(), height = GetScreenHeight();
                sscanf(res.c_str(), "%dx%d", &width, &height);
                SetWindowSize(width, height);
            }
            else if (label == "Back") {
                m_state = MenuState::Options;
            }
        }


        if (label == "Display Mode" && !values.empty())
        {
            std::string mode = values[selectedIndex];
            if (mode == "Fullscreen") {
                SetWindowState(FLAG_FULLSCREEN_MODE);
            }
            else if (mode == "Windowed") {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
            }
            else if (mode == "Borderless") {
                SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_FULLSCREEN_MODE);
            }
        }
        else if (label == "VSync" && !values.empty())
        {
            std::string vsync = values[selectedIndex];
            if (vsync == "On") {
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            }
            else {
                SetTargetFPS(0);
            }
        }
    }

    // Footer
    std::string footer = "[Enter] Apply/Select [←/→] Change [↑/↓] Navigate [Esc] Back";
    int fw = MeasureText(footer.c_str(), 24);
    DrawText(footer.c_str(), GetScreenWidth() / 2 - fw / 2, GetScreenHeight() - 40, 24, GRAY);
}



