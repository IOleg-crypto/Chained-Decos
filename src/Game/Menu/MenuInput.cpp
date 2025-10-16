#include "Menu.h"
#include <raylib.h>
#include <iostream>

// Input handling functionality
void Menu::HandleKeyboardNavigation()
{
    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        HandleMainMenuKeyboardNavigation();
        break;
    case MenuState::Gameplay:
        HandleGameplayNavigation();
        break;
    case MenuState::Video:
        HandleVideoMenuKeyboardNavigation();
        break;
    case MenuState::Credits:
    case MenuState::Mods:
        HandleSimpleScreenKeyboardNavigation();
        break;
    case MenuState::MapSelection:
        HandleMapSelectionKeyboardNavigation();
        break;
    case MenuState::ConfirmExit:
        HandleConfirmExitKeyboardNavigation();
        break;
    default:
        break;
    }
}

void Menu::HandleMainMenuKeyboardNavigation()
{
    const std::vector<MenuItem>* menuToUse = GetCurrentMenuWithDynamicItems();
    if (!menuToUse)
        return;

    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % menuToUse->size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + menuToUse->size() - 1) % menuToUse->size();

    if (IsKeyPressed(KEY_ENTER))
    {
        // Ensure m_selected is within bounds before accessing vector
        if (m_selected >= 0 && m_selected < static_cast<int>(menuToUse->size()))
        {
            m_action = (*menuToUse)[m_selected].action;
        }
    }

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
        auto &opt = m_videoOptions[m_selected];
        if (IsKeyPressed(KEY_RIGHT))
            opt.selectedIndex = (opt.selectedIndex + 1) % opt.values.size();
        if (IsKeyPressed(KEY_LEFT))
            opt.selectedIndex = (opt.selectedIndex + opt.values.size() - 1) % opt.values.size();
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        auto &opt = m_videoOptions[m_selected];
        if (opt.label == "Back")
            m_state = MenuState::Options;
        else if (opt.label == "Resolution")
        {
            int w = 0, h = 0;
            sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &w, &h);
            m_config.SetResolution(w, h);
            SetWindowSize(w, h);
        }
        else if (opt.label == "Display Mode")
        {
            std::string mode = opt.values[opt.selectedIndex];
            bool fullscreen = (mode == "Fullscreen" || mode == "Borderless");
            m_config.SetFullscreen(fullscreen);

            if (mode == "Fullscreen")
                SetWindowState(FLAG_FULLSCREEN_MODE);
            else if (mode == "Windowed")
            {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
            }
            else if (mode == "Borderless")
                SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
        }
        else if (opt.label == "VSync")
        {
            bool vsync = (opt.values[opt.selectedIndex] == "On");
            m_config.SetVSync(vsync);

            if (vsync)
            {
                SetWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(0);
            }
            else
            {
                ClearWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            }
        }
        else if (opt.label == "Target FPS")
        {
            const std::string val = opt.values[opt.selectedIndex];
            ClearWindowState(FLAG_VSYNC_HINT);

            int fps = (val == "Unlimited") ? 0 : std::stoi(val);
            SetTargetFPS(fps);
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Options;

    // Handle volume adjustments in Audio menu
    if (m_state == MenuState::Audio)
    {
        const float volumeStep = 0.1f;

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
        {
            float* currentVolume = nullptr;
            std::string volumeName;

            switch (m_selected)
            {
                case 0: // Master Volume
                    currentVolume = &m_masterVolume;
                    volumeName = "Master";
                    break;
                case 1: // Music Volume
                    currentVolume = &m_musicVolume;
                    volumeName = "Music";
                    break;
                case 2: // SFX Volume
                    currentVolume = &m_sfxVolume;
                    volumeName = "SFX";
                    break;
            }

            if (currentVolume)
            {
                if (IsKeyPressed(KEY_RIGHT) && *currentVolume < 1.0f)
                {
                    *currentVolume = std::min(1.0f, *currentVolume + volumeStep);
                    AddConsoleOutput(volumeName + " volume: " + std::to_string(static_cast<int>(*currentVolume * 100)) + "%");
                }
                else if (IsKeyPressed(KEY_LEFT) && *currentVolume > 0.0f)
                {
                    *currentVolume = std::max(0.0f, *currentVolume - volumeStep);
                    AddConsoleOutput(volumeName + " volume: " + std::to_string(static_cast<int>(*currentVolume * 100)) + "%");
                }
            }
        }
    }

    // Handle sensitivity adjustments in Controls menu
    if (m_state == MenuState::Controls && m_selected == 1) // Mouse Sensitivity option
    {
        const float sensitivityStep = 0.1f;

        if (IsKeyPressed(KEY_LEFT) && m_mouseSensitivity > 0.1f)
        {
            m_mouseSensitivity = std::max(0.1f, m_mouseSensitivity - sensitivityStep);
            AddConsoleOutput("Mouse sensitivity: " + std::to_string(m_mouseSensitivity));
        }
        else if (IsKeyPressed(KEY_RIGHT) && m_mouseSensitivity < 3.0f)
        {
            m_mouseSensitivity = std::min(3.0f, m_mouseSensitivity + sensitivityStep);
            AddConsoleOutput("Mouse sensitivity: " + std::to_string(m_mouseSensitivity));
        }
    }
}

void Menu::HandleVideoMenuKeyboardNavigation()
{
    HandleVideoNavigation();
}

void Menu::ApplyGameplayOption(MenuOption& opt)
{
    if (opt.label == "Difficulty")
    {
        switch (opt.selectedIndex)
        {
            case 0: m_difficultyLevel = 1; break; // Easy
            case 1: m_difficultyLevel = 2; break; // Medium
            case 2: m_difficultyLevel = 3; break; // Hard
        }
        AddConsoleOutput("Difficulty set to: " + opt.values[opt.selectedIndex]);
    }
    else if (opt.label == "Timer")
    {
        m_timerEnabled = (opt.values[opt.selectedIndex] == "On");
        AddConsoleOutput("Timer: " + opt.values[opt.selectedIndex]);
    }
    else if (opt.label == "Checkpoints")
    {
        m_checkpointsEnabled = (opt.values[opt.selectedIndex] == "On");
        AddConsoleOutput("Checkpoints: " + opt.values[opt.selectedIndex]);
    }
    else if (opt.label == "Auto Save")
    {
        m_autoSaveEnabled = (opt.values[opt.selectedIndex] == "On");
        AddConsoleOutput("Auto Save: " + opt.values[opt.selectedIndex]);
    }
    else if (opt.label == "Speedrun Mode")
    {
        m_speedrunMode = (opt.values[opt.selectedIndex] == "On");
        AddConsoleOutput("Speedrun Mode: " + opt.values[opt.selectedIndex]);
    }
}

void Menu::HandleGameplayNavigation()
{
    if (IsKeyPressed(KEY_DOWN))
        m_selected = (m_selected + 1) % m_gameplayOptions.size();
    if (IsKeyPressed(KEY_UP))
        m_selected = (m_selected + m_gameplayOptions.size() - 1) % m_gameplayOptions.size();

    if (m_selected >= 0 && m_selected < static_cast<int>(m_gameplayOptions.size()))
    {
        if (!m_gameplayOptions[m_selected].values.empty())
        {
            auto &opt = m_gameplayOptions[m_selected];
            if (IsKeyPressed(KEY_RIGHT))
                opt.selectedIndex = (opt.selectedIndex + 1) % opt.values.size();
            if (IsKeyPressed(KEY_LEFT))
                opt.selectedIndex = (opt.selectedIndex + opt.values.size() - 1) % opt.values.size();
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            auto &opt = m_gameplayOptions[m_selected];
            if (opt.label == "Back")
            {
                m_state = MenuState::Options;
            }
            else
            {
                // Apply the selected option immediately
                ApplyGameplayOption(opt);
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Options;
}

void Menu::HandleSimpleScreenKeyboardNavigation()
{
    // For Credits and Mods screens - ESC goes back to main menu
    if (IsKeyPressed(KEY_ESCAPE))
        m_state = MenuState::Main;
}

void Menu::HandleConfirmExitKeyboardNavigation()
{
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER))
    {
        m_action = MenuAction::ExitGame;
        if (m_engine)
            m_engine->RequestExit();
    }
    else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleMapSelectionKeyboardNavigation()
{
    if (m_availableMaps.empty())
        return;

    // Get current page map range
    int startIndex = GetStartMapIndex();
    int endIndex = GetEndMapIndex();
    int mapsOnCurrentPage = endIndex - startIndex;

    // Horizontal navigation within current page (left/right only)
    if (IsKeyPressed(KEY_LEFT))
    {
        if (m_selectedMap > startIndex)
            m_selectedMap--;
        else if (m_currentPage > 0)
        {
            // Move to previous page and select last map on that page
            PreviousPage();
            UpdatePagination();
            startIndex = GetStartMapIndex();
            m_selectedMap = std::min(startIndex + mapsOnCurrentPage - 1, static_cast<int>(m_availableMaps.size()) - 1);
        }
        else
        {
            // Wrap to last page and select last map
            m_currentPage = m_totalPages - 1;
            UpdatePagination();
            startIndex = GetStartMapIndex();
            m_selectedMap = std::min(startIndex + mapsOnCurrentPage - 1, static_cast<int>(m_availableMaps.size()) - 1);
        }

        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Map selection changed to: %d (Page %d/%d)",
                 m_selectedMap, m_currentPage + 1, m_totalPages);
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        if (m_selectedMap < endIndex - 1)
            m_selectedMap++;
        else if (m_currentPage < m_totalPages - 1)
        {
            // Move to next page and select first map on that page
            NextPage();
            UpdatePagination();
            m_selectedMap = GetStartMapIndex();
        }
        else
        {
            // Wrap to first page and select first map
            m_currentPage = 0;
            UpdatePagination();
            m_selectedMap = 0;
        }

        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Map selection changed to: %d (Page %d/%d)",
                 m_selectedMap, m_currentPage + 1, m_totalPages);
    }

    // Page navigation
    if (IsKeyPressed(KEY_PAGE_DOWN) || (IsKeyPressed(KEY_RIGHT) && IsKeyDown(KEY_LEFT_CONTROL)))
    {
        NextPage();
        UpdatePagination();
        m_selectedMap = GetStartMapIndex();
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Next page: %d/%d, selected map: %d",
                 m_currentPage + 1, m_totalPages, m_selectedMap);
    }

    if (IsKeyPressed(KEY_PAGE_UP) || (IsKeyPressed(KEY_LEFT) && IsKeyDown(KEY_LEFT_CONTROL)))
    {
        PreviousPage();
        UpdatePagination();
        m_selectedMap = GetStartMapIndex();
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Previous page: %d/%d, selected map: %d",
                 m_currentPage + 1, m_totalPages, m_selectedMap);
    }

    if (IsKeyPressed(KEY_ENTER) && !m_availableMaps.empty())
    {
        // Start the game with selected map
        const MapInfo* selectedMap = GetSelectedMap();
        if (selectedMap)
        {
            TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Starting game with map: %s", selectedMap->displayName.c_str());
            // Set action to start game with selected map
            m_action = MenuAction::StartGameWithMap;
        }
        else
        {
            TraceLog(LOG_ERROR, "Menu::HandleMapSelectionKeyboardNavigation() - No map selected!");
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        TraceLog(LOG_INFO, "Menu::HandleMapSelectionKeyboardNavigation() - Returning to game mode menu");
        m_state = MenuState::GameMode;
    }
}

void Menu::HandleConfirmExit()
{
    if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER))
    {
        m_action = MenuAction::ExitGame;
        // ExecuteAction will handle the actual exit
    }
    else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE))
    {
        m_state = MenuState::Main;
        ResetAction();
    }
}

void Menu::HandleMapSelection()
{
    // Map selection keyboard handling is now done in HandleMapSelectionKeyboardNavigation()
    // This function is kept for potential future non-keyboard input handling
}

void Menu::HandleMouseSelection()
{
    Vector2 mousePos = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        HandleMainMenuMouseSelection(mousePos, clicked);
        break;
    case MenuState::Gameplay:
        HandleGameplayMouseSelection(mousePos, clicked);
        break;
    case MenuState::Video:
        HandleVideoMenuMouseSelection(mousePos, clicked);
        break;
    case MenuState::Credits:
        HandleCreditsMouseSelection(mousePos, clicked);
        break;
    case MenuState::Mods:
        HandleModsMouseSelection(mousePos, clicked);
        break;
    case MenuState::MapSelection:
        HandleMapSelectionMouseSelection(mousePos, clicked);
        break;
    case MenuState::ConfirmExit:
        HandleConfirmExitMouseSelection(mousePos, clicked);
        break;
    default:
        break;
    }
}

void Menu::HandleMainMenuMouseSelection(Vector2 mousePos, bool clicked)
{
    const std::vector<MenuItem>* menuToUse = GetCurrentMenuWithDynamicItems();
    if (!menuToUse)
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Scale button dimensions based on screen size
    float scaleFactor = screenWidth / 1920.0f;  // Base on 1920p
    int kBtnW = static_cast<int>(280 * scaleFactor);
    int kBtnH = static_cast<int>(65 * scaleFactor);
    int kStartY = static_cast<int>(320 * scaleFactor);
    int kSpacing = static_cast<int>(85 * scaleFactor);

    // Ensure minimum usable sizes
    if (kBtnW < 200) kBtnW = 200;
    if (kBtnH < 50) kBtnH = 50;
    if (kStartY < 250) kStartY = 250;
    if (kSpacing < 60) kSpacing = 60;

    for (size_t i = 0; i < menuToUse->size(); ++i)
    {
        int x = screenWidth / 2 - kBtnW / 2;
        int y = kStartY + static_cast<int>(i) * kSpacing;
        Rectangle rect = {(float)x, (float)y, (float)kBtnW, (float)kBtnH};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                // Ensure m_selected is within bounds before accessing vector
                if (m_selected >= 0 && m_selected < static_cast<int>(menuToUse->size()))
                {
                    m_action = (*menuToUse)[m_selected].action;
                }
                break;
            }
        }
    }
}

void Menu::HandleGameplayMouseSelection(Vector2 mousePos, bool clicked)
{
    constexpr int startY = 150, spacing = 80;

    for (size_t i = 0; i < m_gameplayOptions.size(); ++i)
    {
        int y = startY + static_cast<int>(i) * spacing;
        Rectangle rect = {60.0f, (float)(y - 5), (float)(GetScreenWidth() - 120), (float)(spacing - 10)};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                // Ensure m_selected is within bounds before accessing vector
                if (m_selected >= 0 && m_selected < static_cast<int>(m_gameplayOptions.size()))
                {
                    auto &opt = m_gameplayOptions[m_selected];
                    if (opt.label == "Back")
                        m_state = MenuState::Options;
                    else
                        ApplyGameplayOption(opt);
                }
            }
            break;
        }
    }
}

void Menu::HandleVideoMenuMouseSelection(Vector2 mousePos, bool clicked)
{
    constexpr int startY = 150, spacing = 80;

    for (size_t i = 0; i < m_videoOptions.size(); ++i)
    {
        int y = startY + static_cast<int>(i) * spacing;
        Rectangle rect = {60.0f, (float)(y - 5), (float)(GetScreenWidth() - 120), (float)(spacing - 10)};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selected = static_cast<int>(i);
            if (clicked)
            {
                auto &opt = m_videoOptions[m_selected];
                if (opt.label == "Back")
                    m_state = MenuState::Options;
                else if (opt.label == "Resolution")
                {
                    int w = 0, h = 0;
                    sscanf(opt.values[opt.selectedIndex].c_str(), "%dx%d", &w, &h);
                    m_config.SetResolution(w, h);
                    SetWindowSize(w, h);
                }
                else if (opt.label == "Display Mode")
                {
                    std::string mode = opt.values[opt.selectedIndex];
                    bool fullscreen = (mode == "Fullscreen" || mode == "Borderless");
                    m_config.SetFullscreen(fullscreen);

                    if (mode == "Fullscreen")
                        SetWindowState(FLAG_FULLSCREEN_MODE);
                    else if (mode == "Windowed")
                    {
                        ClearWindowState(FLAG_FULLSCREEN_MODE);
                        ClearWindowState(FLAG_WINDOW_UNDECORATED);
                    }
                    else if (mode == "Borderless")
                        SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
                }
                else if (opt.label == "VSync")
                {
                    bool vsync = (opt.values[opt.selectedIndex] == "On");
                    m_config.SetVSync(vsync);

                    if (vsync)
                    {
                        SetWindowState(FLAG_VSYNC_HINT);
                        SetTargetFPS(0);
                    }
                    else
                    {
                        ClearWindowState(FLAG_VSYNC_HINT);
                        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
                    }
                }
                else if (opt.label == "Target FPS")
                {
                    const std::string val = opt.values[opt.selectedIndex];
                    ClearWindowState(FLAG_VSYNC_HINT);

                    int fps = (val == "Unlimited") ? 0 : std::stoi(val);
                    SetTargetFPS(fps);
                }
            }
            break;
        }
    }
}

void Menu::HandleCreditsMouseSelection(Vector2 mousePos, bool clicked)
{
    // Credits screen - mouse click anywhere goes back to main menu
    if (clicked)
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleModsMouseSelection(Vector2 mousePos, bool clicked)
{
    // Mods screen - mouse click anywhere goes back to main menu
    if (clicked)
    {
        m_state = MenuState::Main;
    }
}

void Menu::HandleMapSelectionMouseSelection(Vector2 mousePos, bool clicked)
{
    if (m_availableMaps.empty())
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    float scaleFactor = screenWidth / 1920.0f;

    // Card-based layout - horizontal arrangement
    const int numCards = 3;
    int cardWidth = static_cast<int>(400 * scaleFactor);
    int cardHeight = static_cast<int>(280 * scaleFactor);
    int spacing = static_cast<int>(60 * scaleFactor);
    int startY = static_cast<int>(140 * scaleFactor);

    // Ensure minimum usable sizes
    if (cardWidth < 300) cardWidth = 300;
    if (cardHeight < 200) cardHeight = 200;
    if (spacing < 40) spacing = 40;
    if (startY < 120) startY = 120;

    int totalWidth = numCards * cardWidth + (numCards - 1) * spacing;
    int startX = (screenWidth - totalWidth) / 2;

    // Check each card for mouse interaction
    for (int i = 0; i < static_cast<int>(m_availableMaps.size()); ++i)
    {
        int x = startX + i * (cardWidth + spacing);
        int y = startY;

        // Apply hover/selection scaling
        bool isSelected = (i == m_selectedMap);
        float cardScale = isSelected ? 1.05f : 1.0f;

        int scaledWidth = static_cast<int>(cardWidth * cardScale);
        int scaledHeight = static_cast<int>(cardHeight * cardScale);
        int cardX = x + (cardWidth - scaledWidth) / 2;
        int cardY = y + (cardHeight - scaledHeight) / 2;

        Rectangle rect = {(float)cardX, (float)cardY, (float)scaledWidth, (float)scaledHeight};

        if (CheckCollisionPointRec(mousePos, rect))
        {
            m_selectedMap = i;
            if (clicked)
            {
                // Start the game with selected map
                const MapInfo* selectedMap = GetSelectedMap();
                if (selectedMap)
                {
                    std::cout << "Starting game with map: " << selectedMap->name << std::endl;
                    // Set action to start game with selected map
                    m_action = MenuAction::StartGameWithMap;
                }
            }
            break;
        }
    }

    // Handle back button click
    float backFontSize = 24.0f * scaleFactor;
    if (backFontSize < 18.0f) backFontSize = 18.0f;
    if (backFontSize > 32.0f) backFontSize = 32.0f;

    const char* backText = "Back";
    int backW = MeasureTextEx(m_font, backText, backFontSize, 2.0f).x;
    int backX = static_cast<int>(30 * scaleFactor);
    if (backX < 20) backX = 20;
    int backY = GetScreenHeight() - static_cast<int>(50 * scaleFactor);
    if (backY > GetScreenHeight() - 40) backY = GetScreenHeight() - 40;

    Rectangle backRect = {(float)backX - 10, (float)backY - 5, (float)backW + 20, (float)backFontSize + 10};

    if (CheckCollisionPointRec(mousePos, backRect) && clicked)
    {
        m_state = MenuState::GameMode;
    }
}

void Menu::HandleConfirmExitMouseSelection(Vector2 mousePos, bool clicked)
{
    if (!clicked)
        return;

    // Dynamic sizing based on screen resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;

    int modalWidth = static_cast<int>(500 * scaleFactor);
    int modalHeight = static_cast<int>(300 * scaleFactor);

    // Ensure minimum usable sizes
    if (modalWidth < 400) modalWidth = 400;
    if (modalHeight < 250) modalHeight = 250;
    if (modalWidth > screenWidth - 100) modalWidth = screenWidth - 100;
    if (modalHeight > screenHeight - 100) modalHeight = screenHeight - 100;

    int modalX = screenWidth / 2 - modalWidth / 2;
    int modalY = screenHeight / 2 - modalHeight / 2;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - static_cast<int>(100 * scaleFactor);
    int buttonY = modalY + modalHeight - static_cast<int>(80 * scaleFactor);
    int buttonWidth = static_cast<int>(80 * scaleFactor);
    int buttonHeight = static_cast<int>(40 * scaleFactor);

    if (buttonWidth < 60) buttonWidth = 60;
    if (buttonHeight < 32) buttonHeight = 32;
    if (yesX < modalX + 20) yesX = modalX + 20;

    Rectangle yesRect = {(float)(yesX - 15), (float)(buttonY - 10), (float)buttonWidth, (float)buttonHeight};

    // NO button (right)
    int noX = modalX + modalWidth / 2 + static_cast<int>(40 * scaleFactor);
    if (noX + buttonWidth + 15 > modalX + modalWidth - 20) noX = modalX + modalWidth - buttonWidth - 35;
    Rectangle noRect = {(float)(noX - 15), (float)(buttonY - 10), (float)buttonWidth, (float)buttonHeight};

    if (CheckCollisionPointRec(mousePos, yesRect))
    {
        m_action = MenuAction::ExitGame;
        // ExecuteAction will handle the actual exit
    }
    else if (CheckCollisionPointRec(mousePos, noRect))
    {
        m_state = MenuState::Main;
        ResetAction();
    }
}