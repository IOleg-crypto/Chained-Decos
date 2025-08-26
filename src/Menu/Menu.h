#ifndef MENU_H
#define MENU_H

#include <imgui.h>
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <vector>

// # ----------------------------------------------------------------------------
// # Menu - handles rendering and logic for main menu
// # ----------------------------------------------------------------------------

class Engine;

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
    Video,    // new
    Audio,    // new
    Controls, // new
    Credits,
    Mods
};

struct MenuItem
{
    const char *label;
    MenuAction action;
};

class Menu
{
private:
    int m_selected = 0;
    MenuAction m_action = MenuAction::None;
    MenuState m_state = MenuState::Main;
    std::vector<float> m_buttonScales;
    std::vector<MenuItem> m_currentMenu;
    Engine *m_engine;

private:
    std::vector<MenuItem> m_mainMenu;
    std::vector<MenuItem> m_optionsMenu;
    std::vector<MenuItem> m_SetGameMode;
    std::vector<MenuItem> m_videoMenu;
    std::vector<MenuItem> m_audioMenu;
    std::vector<MenuItem> m_controlsMenu;

public:
    Menu();

public:
    void Update();
    void Render() const;
    void GetEngine(Engine *engine);
    float Lerp(float a, float b, float t) const;
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();
    void RenderSettingsMenu() const;
};

#endif