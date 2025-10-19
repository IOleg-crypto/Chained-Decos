#include "Menu.h"
#include "Engine/Engine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>     // sscanf
#include <filesystem> // For directory scanning
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <string>
#include <vector>

// Include raylib window functions
// Note: GLFW dependency removed as raylib provides all necessary window management

// Simplified menu rendering with core functionality
void Menu::Render()
{
    // Simple dark background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{30, 30, 40, 255});

    // Route to appropriate rendering function based on state
    switch (m_state)
    {
    case MenuState::Main:
    case MenuState::Options:
    case MenuState::GameMode:
    case MenuState::Audio:
    case MenuState::Controls:
        RenderMenu();
        break;
    case MenuState::Gameplay:
        RenderGameplayMenu();
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
    case MenuState::MapSelection:
        RenderMapSelection();
        break;
    case MenuState::ConfirmExit:
        RenderConfirmExit();
        break;
    default:
        break;
    }

    // Render console if open
    if (m_consoleOpen)
    {
        RenderConsole();
    }
}

// Console functions that are specific to Menu.cpp
void Menu::ToggleConsole()
{
    m_consoleOpen = !m_consoleOpen;
    if (m_consoleOpen)
    {
        m_consoleInput.clear();
        m_consoleHistoryIndex = m_consoleHistory.size();
    }
}

void Menu::HandleConsoleInput()
{
    if (!m_consoleOpen)
        return;

    int key = GetCharPressed();
    while (key > 0)
    {
        if (key >= 32 && key <= 125) // Printable characters
        {
            m_consoleInput += (char)key;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !m_consoleInput.empty())
    {
        m_consoleInput.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER) && !m_consoleInput.empty())
    {
        // Add to history
        m_consoleHistory.push_back(m_consoleInput);
        if (m_consoleHistory.size() > MAX_HISTORY_LINES)
        {
            m_consoleHistory.erase(m_consoleHistory.begin());
        }

        // Execute command
        ExecuteConsoleCommand(m_consoleInput);

        // Clear input
        m_consoleInput.clear();
        m_consoleHistoryIndex = m_consoleHistory.size();
    }

    if (IsKeyPressed(KEY_UP) && !m_consoleHistory.empty())
    {
        if (m_consoleHistoryIndex > 0)
        {
            m_consoleHistoryIndex--;
            m_consoleInput = m_consoleHistory[m_consoleHistoryIndex];
        }
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        if (m_consoleHistoryIndex < m_consoleHistory.size() - 1)
        {
            m_consoleHistoryIndex++;
            m_consoleInput = m_consoleHistory[m_consoleHistoryIndex];
        }
        else if (m_consoleHistoryIndex == m_consoleHistory.size() - 1)
        {
            m_consoleHistoryIndex = m_consoleHistory.size();
            m_consoleInput.clear();
        }
    }
}

void Menu::ExecuteConsoleCommand(const std::string &command)
{
    AddConsoleOutput("] " + command);

    // Simple command parsing
    std::string cmd, args;
    size_t spacePos = command.find(' ');
    if (spacePos != std::string::npos)
    {
        cmd = command.substr(0, spacePos);
        args = command.substr(spacePos + 1);
    }
    else
    {
        cmd = command;
    }

    // Convert to lowercase for case-insensitive comparison
    for (char &c : cmd)
        c = std::tolower(c);

    // Simple command handling
    if (cmd == "help")
    {
        AddConsoleOutput("Commands: help, clear, quit, fps, res, vsync, savecfg");
    }
    else if (cmd == "clear")
    {
        m_consoleOutput.clear();
    }
    else if (cmd == "quit" || cmd == "exit")
    {
        if (m_engine)
            m_engine->RequestExit();
        AddConsoleOutput("Exiting...");
    }
    else if (cmd == "fps")
    {
        AddConsoleOutput("FPS: " + std::to_string(GetFPS()));
    }
    else if (cmd == "res" && !args.empty())
    {
        int width, height;
        if (sscanf(args.c_str(), "%dx%d", &width, &height) == 2)
        {
            SetWindowSize(width, height);
            AddConsoleOutput("Resolution: " + std::to_string(width) + "x" + std::to_string(height));
        }
        else
        {
            AddConsoleOutput("Usage: res <width>x<height>");
        }
    }
    else if (cmd == "vsync")
    {
        if (args == "on" || args == "1")
        {
            SetWindowState(FLAG_VSYNC_HINT);
            AddConsoleOutput("VSync enabled");
        }
        else if (args == "off" || args == "0")
        {
            ClearWindowState(FLAG_VSYNC_HINT);
            AddConsoleOutput("VSync disabled");
        }
        else
        {
            AddConsoleOutput("Usage: vsync <on/off>");
        }
    }
    else if (cmd == "savecfg")
    {
        SaveSettings();
        AddConsoleOutput("Settings saved");
    }
    else
    {
        AddConsoleOutput("Unknown command. Type 'help' for commands.");
    }
}

void Menu::AddConsoleOutput(const std::string &text)
{
    m_consoleOutput.push_back(text);
    if (m_consoleOutput.size() > MAX_CONSOLE_LINES)
    {
        m_consoleOutput.erase(m_consoleOutput.begin());
    }
}

void Menu::RenderConsole() const
{
    if (!m_consoleOpen)
        return;

    const int consoleHeight = GetScreenHeight() / 2;
    const int lineHeight = 18;

    // Semi-transparent overlay
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

    // Console background
    DrawRectangle(0, 0, GetScreenWidth(), consoleHeight, Fade(BLACK, 0.9f));
    DrawRectangleLines(0, 0, GetScreenWidth(), consoleHeight, WHITE);

    // Title and pause indicator
    DrawText("Console", 10, 10, 20, YELLOW);
    DrawText("GAME PAUSED", GetScreenWidth() - 150, 10, 16, RED);

    // Display recent output lines (last 10 lines)
    int startY = 40;
    size_t startLine = (m_consoleOutput.size() > 10) ? m_consoleOutput.size() - 10 : 0;

    for (size_t i = startLine; i < m_consoleOutput.size(); ++i)
    {
        int y = startY + (i - startLine) * lineHeight;
        if (y < consoleHeight - 40)
        { // Don't overflow console area
            DrawText(m_consoleOutput[i].c_str(), 10, y, 16, WHITE);
        }
    }

    // Input line
    int inputY = consoleHeight - 25;
    DrawText("]", 10, inputY, 16, GREEN);
    DrawText(m_consoleInput.c_str(), 25, inputY, 16, WHITE);

    // Simple blinking cursor
    if ((int)(GetTime() * 2) % 2 == 0)
    {
        int cursorX = 25 + MeasureText(m_consoleInput.c_str(), 16);
        DrawText("_", cursorX, inputY, 16, WHITE);
    }

    // Simple instructions
    DrawText("[~] Toggle | [Enter] Execute", GetScreenWidth() - 200, GetScreenHeight() - 20, 14,
             GRAY);
}

// Simplified menu rendering with core functionality preserved
void Menu::RenderMenu()
{
    if (!m_currentMenu)
        return;

    // Simple background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{30, 30, 40, 255});

    Vector2 mousePos = GetMousePosition();
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Get current menu to render
    const std::vector<MenuItem> *menuToRender = GetCurrentMenuWithDynamicItems();
    if (!menuToRender)
        return;

    // Simple title
    const char *title = (m_state == MenuState::Main) ? "CHAINED DECOS"
                        : (m_selected >= 0 && m_selected < static_cast<int>(menuToRender->size()))
                            ? (*menuToRender)[m_selected].label
                            : "MENU";

    int titleFontSize = 48;
    int tw = MeasureText(title, titleFontSize);
    DrawText(title, screenWidth / 2 - tw / 2, 80, titleFontSize, WHITE);

    // Subtitle for main menu
    if (m_state == MenuState::Main)
    {
        const char *subtitle = "3D Platformer";
        int subtitleFontSize = 24;
        int stw = MeasureText(subtitle, subtitleFontSize);
        DrawText(subtitle, screenWidth / 2 - stw / 2, 130, subtitleFontSize,
                 Color{150, 150, 150, 255});
    }

    // Ensure button scales are properly sized
    if (m_buttonScales.size() != menuToRender->size())
    {
        m_buttonScales.assign(menuToRender->size(), 1.0f);
        if (m_selected >= static_cast<int>(menuToRender->size()))
            m_selected = 0;
    }

    // Simple button layout
    int buttonWidth = 280;
    int buttonHeight = 60;
    int startY = 200;
    int spacing = 80;

    for (size_t i = 0; i < menuToRender->size(); ++i)
    {
        const auto &item = (*menuToRender)[i];
        int x = screenWidth / 2 - buttonWidth / 2;
        int y = startY + static_cast<int>(i) * spacing;
        Rectangle btnRect = {(float)x, (float)y, (float)buttonWidth, (float)buttonHeight};

        bool hovered = CheckCollisionPointRec(mousePos, btnRect);
        bool selected = (static_cast<int>(i) == m_selected);

        // Update button scale for smooth animation
        float targetScale = (hovered || selected) ? 1.1f : 1.0f;
        m_buttonScales[i] = Lerp(m_buttonScales[i], targetScale, 0.2f);

        int scaledWidth = static_cast<int>(buttonWidth * m_buttonScales[i]);
        int scaledHeight = static_cast<int>(buttonHeight * m_buttonScales[i]);
        int scaledX = screenWidth / 2 - scaledWidth / 2;
        int scaledY = y - (scaledHeight - buttonHeight) / 2;

        // Button colors
        Color btnColor = selected ? Color{100, 150, 255, 255}
                                  : (hovered ? Color{80, 90, 110, 255} : Color{60, 70, 90, 255});

        // Draw button
        DrawRectangle(scaledX, scaledY, scaledWidth, scaledHeight, btnColor);
        DrawRectangleLines(scaledX, scaledY, scaledWidth, scaledHeight, Color{120, 130, 150, 255});

        // Draw text
        int textSize = selected ? 28 : 24;
        Color textColor = selected ? Color{255, 255, 255, 255} : Color{200, 210, 230, 255};
        int textW = MeasureText(item.label, textSize);
        int textX = scaledX + scaledWidth / 2 - textW / 2;
        int textY = scaledY + scaledHeight / 2 - textSize / 2;
        DrawText(item.label, textX, textY, textSize, textColor);
    }

    // Simple footer
    const char *footer = "ENTER: Select | ESC: Back | ↑↓: Navigate";
    int footerFontSize = 18;
    int fw = MeasureText(footer, footerFontSize);
    DrawText(footer, screenWidth / 2 - fw / 2, screenHeight - 40, footerFontSize,
             Color{150, 150, 150, 255});
}

// Additional essential rendering functions
void Menu::RenderSettingsMenu() const
{
    int startY = 150, spacing = 80, fontSize = 30;

    // Modern settings title with glow
    const char *settingsTitle = "SETTINGS";
    int titleW = MeasureTextEx(m_font, settingsTitle, 45, 2.0f).x;
    int titleX = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX + i, (float)45 + i}, 45, 2.0f,
                   Fade(Color{255, 150, 50, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX, (float)45}, 45, 2.0f,
               Color{255, 200, 100, 255});

    for (size_t i = 0; i < m_videoOptions.size(); ++i)
    {
        auto &opt = m_videoOptions[i];
        int y = startY + static_cast<int>(i) * spacing;

        bool isSelected = (static_cast<int>(i) == m_selected);
        Color labelColor = isSelected ? Color{255, 220, 150, 255} : Color{200, 210, 230, 255};

        // Modern setting container
        if (isSelected)
        {
            // Background highlight for selected option
            DrawRectangle(60, y - 5, GetScreenWidth() - 120, spacing - 10,
                          Fade(Color{100, 150, 255, 255}, 0.2f));
            DrawRectangleLines(60, y - 5, GetScreenWidth() - 120, spacing - 10,
                               Color{150, 200, 255, 255});
        }

        // Draw setting label with modern typography
        const char *label = opt.label.c_str();
        int labelW = MeasureTextEx(m_font, label, fontSize, 2.0f).x;
        DrawTextEx(m_font, label, Vector2{80.0f, (float)y + 5}, fontSize, 2.0f, labelColor);

        if (!opt.values.empty())
        {
            // Get current system value for display
            std::string currentValue = GetCurrentSettingValue(opt.label);
            if (!currentValue.empty())
            {
                int currentWidth =
                    MeasureTextEx(m_font, currentValue.c_str(), fontSize - 8, 2.0f).x;
                DrawTextEx(m_font, currentValue.c_str(), Vector2{(float)80 + 320, (float)y + 8},
                           fontSize - 8, 2.0f,
                           isSelected ? Fade(Color{255, 255, 150, 255}, 0.9f)
                                      : Fade(Color{180, 200, 150, 255}, 0.7f));
            }

            // Show selected value with modern styling
            std::string displayValue;
            if (opt.selectedIndex < opt.values.size())
            {
                displayValue = opt.values[opt.selectedIndex];
            }

            if (!displayValue.empty())
            {
                // Modern value display with background
                int textWidth = MeasureTextEx(m_font, displayValue.c_str(), fontSize, 2.0f).x;
                int xPos = GetScreenWidth() - textWidth - 100;

                if (isSelected)
                {
                    // Background for selected value
                    DrawRectangle(xPos - 10, y - 2, textWidth + 20, fontSize + 8,
                                  Fade(Color{255, 200, 100, 255}, 0.3f));
                    DrawRectangleLines(xPos - 10, y - 2, textWidth + 20, fontSize + 8,
                                       Color{255, 180, 80, 255});
                }

                DrawTextEx(m_font, displayValue.c_str(), Vector2{(float)xPos, (float)y + 5},
                           fontSize, 2.0f,
                           isSelected ? Color{255, 255, 180, 255} : Color{220, 230, 200, 255});
            }
        }
    }

    // Modern settings footer
    std::string footer = "ENTER Apply/Select    Arrow L/R Change    ↑↓ Navigate    ESC Back";
    int fw = MeasureTextEx(m_font, footer.c_str(), 18, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 30;

    // Footer background
    DrawRectangle(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw + 16, 26,
                       Fade(Color{120, 140, 160, 255}, 0.5f));

    // Color-coded footer text
    DrawTextEx(m_font, "ENTER", Vector2{(float)footerX, (float)footerY}, 18, 2.0f,
               Color{150, 255, 150, 255});
    DrawTextEx(m_font, " Apply/Select    ", Vector2{(float)footerX + 65, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "←→", Vector2{(float)footerX + 190, (float)footerY}, 18, 2.0f,
               Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Change    ", Vector2{(float)footerX + 210, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "↑↓", Vector2{(float)footerX + 290, (float)footerY}, 18, 2.0f,
               Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Navigate    ", Vector2{(float)footerX + 310, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "ESC", Vector2{(float)footerX + 410, (float)footerY}, 18, 2.0f,
               Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 440, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
}

void Menu::RenderCredits()
{
    // Modern credits title with glow
    const char *title = "CREDITS";
    int tw = MeasureTextEx(m_font, title, 50, 2.0f).x;
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)titleX + i, (float)titleY + i}, 50, 2.0f,
                   Fade(Color{255, 150, 100, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, title, Vector2{(float)titleX, (float)titleY}, 50, 2.0f,
               Color{255, 200, 150, 255});

    // Modern credits content with better layout
    int y = 180, fs = 28;

    // Developer credit with modern styling
    DrawTextEx(m_font, "DEVELOPER", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "I#Oleg", Vector2{80.0f, (float)y + 30}, fs, 2.0f,
               Color{255, 255, 200, 255});

    y += 100;
    DrawTextEx(m_font, "ENGINE", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "raylib + rlImGui", Vector2{80.0f, (float)y + 30}, fs, 2.0f,
               Color{255, 255, 200, 255});

    y += 100;
    DrawTextEx(m_font, "UI DESIGN", Vector2{80.0f, (float)y}, 24, 2.0f, Color{200, 220, 255, 255});
    DrawTextEx(m_font, "Modern Interface", Vector2{80.0f, (float)y + 30}, fs, 2.0f,
               Color{255, 255, 200, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureTextEx(m_font, footer, 20, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26,
                       Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawTextEx(m_font, "ESC", Vector2{(float)footerX, (float)footerY}, 20, 2.0f,
               Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 35, (float)footerY}, 20, 2.0f,
               Color{200, 200, 200, 255});
}

void Menu::RenderMods()
{
    // Modern mods title with glow
    const char *title = "MODS";
    int tw = MeasureTextEx(m_font, title, 50, 2.0f).x;
    int titleX = GetScreenWidth() / 2 - tw / 2;
    int titleY = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, title, Vector2{(float)titleX + i, (float)titleY + i}, 50, 2.0f,
                   Fade(Color{200, 100, 255, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, title, Vector2{(float)titleX, (float)titleY}, 50, 2.0f,
               Color{220, 150, 255, 255});

    // Modern content layout
    int y = 180, fs = 26;

    // No mods message with modern styling
    const char *noModsMsg = "NO MODS DETECTED";
    int noModsW = MeasureTextEx(m_font, noModsMsg, 28, 2.0f).x;
    int noModsX = GetScreenWidth() / 2 - noModsW / 2;
    DrawTextEx(m_font, noModsMsg, Vector2{(float)noModsX, (float)y}, 28, 2.0f,
               Color{255, 200, 150, 255});

    y += 80;
    const char *instructionMsg = "Place your mods in the 'resources/mods' folder";
    int instructionW = MeasureTextEx(m_font, instructionMsg, fs, 2.0f).x;
    int instructionX = GetScreenWidth() / 2 - instructionW / 2;
    DrawTextEx(m_font, instructionMsg, Vector2{(float)instructionX, (float)y}, fs, 2.0f,
               Color{180, 200, 220, 255});

    // Modern footer
    const char *footer = "ESC Back";
    int fw2 = MeasureTextEx(m_font, footer, 20, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw2 / 2;
    int footerY = GetScreenHeight() - 30;

    DrawRectangle(footerX - 8, footerY - 3, fw2 + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw2 + 16, 26,
                       Fade(Color{120, 140, 160, 255}, 0.5f));

    DrawTextEx(m_font, "ESC", Vector2{(float)footerX, (float)footerY}, 20, 2.0f,
               Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 35, (float)footerY}, 20, 2.0f,
               Color{200, 200, 200, 255});
}

void Menu::RenderGameplayMenu()
{
    int startY = 150, spacing = 80, fontSize = 30;

    // Modern gameplay settings title with glow
    const char *settingsTitle = "GAMEPLAY SETTINGS";
    int titleW = MeasureTextEx(m_font, settingsTitle, 45, 2.0f).x;
    int titleX = 80;

    // Title glow effect
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX + i, (float)45 + i}, 45, 2.0f,
                   Fade(Color{100, 255, 150, 255}, 0.5f / i));
    }
    DrawTextEx(m_font, settingsTitle, Vector2{(float)titleX, (float)45}, 45, 2.0f,
               Color{150, 255, 200, 255});

    for (size_t i = 0; i < m_gameplayOptions.size(); ++i)
    {
        auto &opt = m_gameplayOptions[i];
        int y = startY + static_cast<int>(i) * spacing;

        bool isSelected = (static_cast<int>(i) == m_selected);
        Color labelColor = isSelected ? Color{255, 220, 150, 255} : Color{200, 210, 230, 255};

        // Modern setting container
        if (isSelected)
        {
            // Background highlight for selected option
            DrawRectangle(60, y - 5, GetScreenWidth() - 120, spacing - 10,
                          Fade(Color{100, 255, 150, 255}, 0.2f));
            DrawRectangleLines(60, y - 5, GetScreenWidth() - 120, spacing - 10,
                               Color{150, 255, 200, 255});
        }

        // Draw setting label with modern typography
        const char *label = opt.label.c_str();
        int labelW = MeasureTextEx(m_font, label, fontSize, 2.0f).x;
        DrawTextEx(m_font, label, Vector2{80.0f, (float)y + 5}, fontSize, 2.0f, labelColor);

        if (!opt.values.empty())
        {
            // Show current value (smaller font, different position)
            std::string currentValue = GetGameplaySettingValue(opt.label);
            if (!currentValue.empty())
            {
                int currentWidth =
                    MeasureTextEx(m_font, currentValue.c_str(), fontSize - 8, 2.0f).x;
                DrawTextEx(m_font, currentValue.c_str(), Vector2{(float)80 + 320, (float)y + 8},
                           fontSize - 8, 2.0f,
                           isSelected ? Fade(Color{255, 255, 150, 255}, 0.9f)
                                      : Fade(Color{180, 200, 150, 255}, 0.7f));
            }

            // Show selected value with modern styling
            std::string displayValue;
            if (opt.selectedIndex >= 0 && opt.selectedIndex < static_cast<int>(opt.values.size()))
            {
                displayValue = opt.values[opt.selectedIndex];
            }

            if (!displayValue.empty())
            {
                // Modern value display with background
                int textWidth = MeasureTextEx(m_font, displayValue.c_str(), fontSize, 2.0f).x;
                int xPos = GetScreenWidth() - textWidth - 100;

                if (isSelected)
                {
                    // Background for selected value
                    DrawRectangle(xPos - 10, y - 2, textWidth + 20, fontSize + 8,
                                  Fade(Color{255, 200, 100, 255}, 0.3f));
                    DrawRectangleLines(xPos - 10, y - 2, textWidth + 20, fontSize + 8,
                                       Color{255, 180, 80, 255});
                }

                DrawTextEx(m_font, displayValue.c_str(), Vector2{(float)xPos, (float)y + 5},
                           fontSize, 2.0f,
                           isSelected ? Color{255, 255, 180, 255} : Color{220, 230, 200, 255});
            }
        }
    }

    // Modern settings footer
    std::string footer = "ENTER Apply/Select    ←→ Change    ↑↓ Navigate    ESC Back";
    int fw = MeasureTextEx(m_font, footer.c_str(), 18, 2.0f).x;
    int footerX = GetScreenWidth() / 2 - fw / 2;
    int footerY = GetScreenHeight() - 30;

    // Footer background
    DrawRectangle(footerX - 8, footerY - 3, fw + 16, 26, Fade(Color{0, 0, 0, 255}, 0.4f));
    DrawRectangleLines(footerX - 8, footerY - 3, fw + 16, 26,
                       Fade(Color{120, 140, 160, 255}, 0.5f));

    // Color-coded footer text
    DrawTextEx(m_font, "ENTER", Vector2{(float)footerX, (float)footerY}, 18, 2.0f,
               Color{150, 255, 150, 255});
    DrawTextEx(m_font, " Apply/Select    ", Vector2{(float)footerX + 65, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "←→", Vector2{(float)footerX + 190, (float)footerY}, 18, 2.0f,
               Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Change    ", Vector2{(float)footerX + 210, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "↑↓", Vector2{(float)footerX + 290, (float)footerY}, 18, 2.0f,
               Color{150, 150, 255, 255});
    DrawTextEx(m_font, " Navigate    ", Vector2{(float)footerX + 310, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
    DrawTextEx(m_font, "ESC", Vector2{(float)footerX + 410, (float)footerY}, 18, 2.0f,
               Color{255, 150, 150, 255});
    DrawTextEx(m_font, " Back", Vector2{(float)footerX + 440, (float)footerY}, 18, 2.0f,
               Color{200, 200, 200, 255});
}

void Menu::RenderConfirmExit()
{
    // Modern modal background with blur effect
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Color{0, 0, 0, 180}, 0.7f));

    // Modal dialog container - dynamic sizing
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float scaleFactor = screenWidth / 1920.0f;

    int modalWidth = static_cast<int>(500 * scaleFactor);
    int modalHeight = static_cast<int>(300 * scaleFactor);

    // Ensure minimum usable sizes
    if (modalWidth < 400)
        modalWidth = 400;
    if (modalHeight < 250)
        modalHeight = 250;
    if (modalWidth > screenWidth - 100)
        modalWidth = screenWidth - 100;
    if (modalHeight > screenHeight - 100)
        modalHeight = screenHeight - 100;

    int modalX = screenWidth / 2 - modalWidth / 2;
    int modalY = screenHeight / 2 - modalHeight / 2;

    // Modal background with gradient
    for (int i = 0; i < modalHeight; i++)
    {
        float t = (float)i / modalHeight;
        Color c = {(unsigned char)(60 + 40 * t), (unsigned char)(40 + 30 * t),
                   (unsigned char)(80 + 60 * t), 220};
        DrawLine(modalX, modalY + i, modalX + modalWidth, modalY + i, c);
    }

    // Modal border with glow
    for (int g = 3; g >= 1; g--)
    {
        DrawRectangleLines(modalX - g, modalY - g, modalWidth + g * 2, modalHeight + g * 2,
                           Fade(Color{150, 100, 255, 255}, 0.3f / g));
    }

    // Modern title - dynamic sizing
    float titleFontSize = 40.0f * scaleFactor;
    if (titleFontSize < 30.0f)
        titleFontSize = 30.0f;
    if (titleFontSize > 60.0f)
        titleFontSize = 60.0f;

    const char *msg = "EXIT GAME?";
    int tw = MeasureTextEx(m_font, msg, titleFontSize, 2.0f).x;
    int titleX = modalX + modalWidth / 2 - tw / 2;
    int titleY = modalY + static_cast<int>(40 * scaleFactor);
    if (titleY < modalY + 30)
        titleY = modalY + 30;

    // Title glow
    for (int i = 2; i >= 1; i--)
    {
        DrawTextEx(m_font, msg, Vector2{(float)titleX + i, (float)titleY + i}, titleFontSize, 2.0f,
                   Fade(Color{255, 100, 100, 255}, 0.6f / i));
    }
    DrawTextEx(m_font, msg, Vector2{(float)titleX, (float)titleY}, titleFontSize, 2.0f,
               Color{255, 150, 150, 255});

    // Modern buttons with better styling - dynamic sizing
    float buttonFontSize = 28.0f * scaleFactor;
    if (buttonFontSize < 22.0f)
        buttonFontSize = 22.0f;
    if (buttonFontSize > 40.0f)
        buttonFontSize = 40.0f;

    const char *yes = "YES";
    const char *no = "NO";
    int yw = MeasureTextEx(m_font, yes, buttonFontSize, 2.0f).x;
    int nw = MeasureTextEx(m_font, no, buttonFontSize, 2.0f).x;

    int buttonY = modalY + modalHeight - static_cast<int>(80 * scaleFactor);
    if (buttonY > modalY + modalHeight - 60)
        buttonY = modalY + modalHeight - 60;

    int buttonHeight = static_cast<int>(40 * scaleFactor);
    if (buttonHeight < 32)
        buttonHeight = 32;

    // YES button (left)
    int yesX = modalX + modalWidth / 2 - yw - static_cast<int>(40 * scaleFactor);
    if (yesX < modalX + 20)
        yesX = modalX + 20;

    DrawRectangle(yesX - 15, buttonY - 10, yw + 30, buttonHeight,
                  Fade(Color{255, 100, 100, 255}, 0.8f));
    DrawRectangleLines(yesX - 15, buttonY - 10, yw + 30, buttonHeight, Color{255, 150, 150, 255});
    DrawTextEx(m_font, yes, Vector2{(float)yesX, (float)buttonY + 2}, buttonFontSize, 2.0f,
               Color{255, 255, 200, 255});

    // NO button (right)
    int noX = modalX + modalWidth / 2 + static_cast<int>(40 * scaleFactor);
    if (noX + nw + 15 > modalX + modalWidth - 20)
        noX = modalX + modalWidth - nw - 35;

    DrawRectangle(noX - 15, buttonY - 10, nw + 30, buttonHeight,
                  Fade(Color{100, 150, 100, 255}, 0.8f));
    DrawRectangleLines(noX - 15, buttonY - 10, nw + 30, buttonHeight, Color{150, 200, 150, 255});
    DrawTextEx(m_font, no, Vector2{(float)noX, (float)buttonY + 2}, buttonFontSize, 2.0f,
               Color{200, 255, 200, 255});

    // Instructions - dynamic sizing
    float instFontSize = 20.0f * scaleFactor;
    if (instFontSize < 16.0f)
        instFontSize = 16.0f;
    if (instFontSize > 28.0f)
        instFontSize = 28.0f;

    const char *instructions = "Y/ENTER = Yes    N/ESC = No";
    int iw = MeasureTextEx(m_font, instructions, instFontSize, 2.0f).x;
    int instX = modalX + modalWidth / 2 - iw / 2;
    int instY = modalY + modalHeight - static_cast<int>(30 * scaleFactor);
    if (instY > modalY + modalHeight - 20)
        instY = modalY + modalHeight - 20;

    DrawTextEx(m_font, instructions, Vector2{(float)instX, (float)instY}, instFontSize, 2.0f,
               Color{180, 190, 210, 255});
}

// Essential helper functions for rendering
std::string Menu::GetGameplaySettingValue(const std::string &settingName) const
{
    if (settingName == "Difficulty")
    {
        switch (m_difficultyLevel)
        {
        case 1:
            return "Easy";
        case 2:
            return "Medium";
        case 3:
            return "Hard";
        default:
            return "Medium";
        }
    }
    else if (settingName == "Timer")
    {
        return m_timerEnabled ? "On" : "Off";
    }
    else if (settingName == "Checkpoints")
    {
        return m_checkpointsEnabled ? "On" : "Off";
    }
    else if (settingName == "Auto Save")
    {
        return m_autoSaveEnabled ? "On" : "Off";
    }
    else if (settingName == "Speedrun Mode")
    {
        return m_speedrunMode ? "On" : "Off";
    }

    return "";
}

std::string Menu::GetCurrentSettingValue(const std::string &settingName) const
{
    if (settingName == "Resolution")
    {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        return TextFormat("%dx%d", width, height);
    }
    else if (settingName == "Display Mode")
    {
        // Check if we're in fullscreen mode
        bool isFullscreen = IsWindowFullscreen();
        if (isFullscreen)
        {
            // Check if window is decorated (not borderless)
            bool isDecorated = !IsWindowState(FLAG_WINDOW_UNDECORATED);
            if (isDecorated)
                return "Fullscreen";
            else
                return "Borderless";
        }
        else
            return "Windowed";
    }
    else if (settingName == "VSync")
    {
        return (FLAG_VSYNC_HINT) ? "On" : "Off";
    }
    else if (settingName == "Target FPS")
    {
        int targetFPS = GetFPS();
        if (targetFPS == 0)
            return "Unlimited";
        else
            return TextFormat("%d", targetFPS);
    }
    else if (settingName == "Aspect Ratio")
    {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        float aspect = (float)width / (float)height;

        if (fabsf(aspect - 16.0f / 9.0f) < 0.1f)
            return "16:9";
        else if (fabsf(aspect - 4.0f / 3.0f) < 0.1f)
            return "4:3";
        else if (fabsf(aspect - 21.0f / 9.0f) < 0.1f)
            return "21:9";
        else
            return TextFormat("%.2f:1", aspect);
    }
    else if (settingName == "Master Volume")
    {
        return TextFormat("%.0f%%", m_masterVolume * 100);
    }
    else if (settingName == "Music Volume")
    {
        return TextFormat("%.0f%%", m_musicVolume * 100);
    }
    else if (settingName == "SFX Volume")
    {
        return TextFormat("%.0f%%", m_sfxVolume * 100);
    }
    else if (settingName == "Mouse Sensitivity")
    {
        return TextFormat("%.1fx", m_mouseSensitivity);
    }
    else if (settingName == "Invert Y Axis")
    {
        return m_invertYAxis ? "On" : "Off";
    }
    else if (settingName == "Controller Support")
    {
        return m_controllerSupport ? "On" : "Off";
    }
    return "";
}

// Pagination methods for map selection
void Menu::UpdatePagination()
{
    if (m_availableMaps.empty())
    {
        m_totalPages = 0;
        m_currentPage = 0;
        return;
    }

    m_totalPages = (static_cast<int>(m_availableMaps.size()) + m_mapsPerPage - 1) / m_mapsPerPage;
    if (m_currentPage >= m_totalPages)
        m_currentPage = std::max(0, m_totalPages - 1);
}

void Menu::NextPage()
{
    if (m_currentPage < m_totalPages - 1)
        m_currentPage++;
}

void Menu::PreviousPage()
{
    if (m_currentPage > 0)
        m_currentPage--;
}

int Menu::GetStartMapIndex() const { return m_currentPage * m_mapsPerPage; }

int Menu::GetEndMapIndex() const
{
    return std::min(GetStartMapIndex() + m_mapsPerPage, static_cast<int>(m_availableMaps.size()));
}

bool Menu::IsVisible() const { return m_state != MenuState::Main || m_gameInProgress; }

// MapInfo constructor implementation
MapInfo::MapInfo(const std::string &name, const std::string &displayName,
                 const std::string &description, const std::string &previewImage, Color themeColor,
                 bool isAvailable, bool isModelBased)
    : name(name), displayName(displayName), description(description), previewImage(previewImage),
      themeColor(themeColor), isAvailable(isAvailable), isModelBased(isModelBased)
{
}
