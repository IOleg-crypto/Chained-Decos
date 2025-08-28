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
    Video,
    Audio,
    Controls,
    Credits,
    Mods
};

struct MenuItem
{
    const char *label;
    MenuAction action;
};

struct MenuOption
{
    std::string label;
    std::vector<std::string> values;
    int selectedIndex;
};

class Menu
{
private:
    int m_selected = 0;
    MenuAction m_action = MenuAction::None;
    MenuState m_state = MenuState::Main;
    mutable std::vector<float> m_buttonScales;
    std::vector<MenuItem> m_currentMenu;
    Engine *m_engine;

private:
    std::vector<MenuItem> m_mainMenu;
    std::vector<MenuItem> m_optionsMenu;
    std::vector<MenuItem> m_SetGameMode;
    std::vector<MenuItem> m_videoMenu;
    std::vector<MenuItem> m_audioMenu;
    std::vector<MenuItem> m_controlsMenu;

    std::vector<MenuOption> m_videoOptions;

public:
    Menu();

public:
    void Update();
    void GetEngine(Engine *engine);
    void Render();
    float Lerp(float a, float b, float t) const;
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();
    void RenderSettingsMenu();
    void RenderCredits();
    void RenderMods();
    void ExecuteAction();
    void HandleMouseSelection();
    void HandleKeyboardNavigation();
    void HandleVideoNavigation();
    void RenderMenu();
};

#endif
