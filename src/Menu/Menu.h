#ifndef MENU_H
#define MENU_H

#include <array>
#include <imgui.h>
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <vector>

// # ----------------------------------------------------------------------------
// # Menu - handles rendering and logic for main menu
// # ----------------------------------------------------------------------------

enum class MenuAction : uint8_t
{
    None,
    StartGame,
    OpenOptions,
    OpenCredits,
    OpenVideoMode,
    OpenAudio,
    OpenControls,
    OpenLanguage,
    OpenMods,
    BackToMainMenu,
    SinglePlayer,
    MultiPlayer,
    OpenGameModeMenu,
    ExitGame
};

enum class MenuState : uint8_t
{
    Main,
    GameMode,
    Options,
    Credits,
    Mods
};

struct MenuItem
{
    const char *label;
    MenuAction action;
};

static std::vector<MenuItem> mainMenu = {{{.label = "Start Game", .action = MenuAction::StartGame},
                                          {.label = "Options", .action = MenuAction::OpenOptions},
                                          {.label = "Credits", .action = MenuAction::OpenCredits},
                                          {.label = "Quit", .action = MenuAction::ExitGame}}};

static std::vector<MenuItem> optionsMenu = {
    {{.label = "Video", .action = MenuAction::OpenVideoMode},
     {.label = "Audio", .action = MenuAction::OpenAudio},
     {.label = "Controls", .action = MenuAction::OpenControls},
     {.label = "Back", .action = MenuAction::BackToMainMenu}}};

static std::vector<MenuItem> SetGameMode = {
    {{.label = "Singleplayer", .action = MenuAction::SinglePlayer},
     {.label = "Multiplayer", .action = MenuAction::MultiPlayer},
     {.label = "Back", .action = MenuAction::BackToMainMenu}}};

class Menu
{
private:
    int m_selected = 0;
    MenuAction m_action = MenuAction::None;
    MenuState m_state = MenuState::Main;
    mutable std::vector<float> m_buttonScales;
    std::vector<MenuItem> m_currentMenu;

public:
    void Update();
    void Render() const;
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();
    float Lerp(float a, float b, float t) const;
};

#endif