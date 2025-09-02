#ifndef MENU_H
#define MENU_H

// ВАЖЛИВО: Переконайтесь, що include path для raylib прописаний у CMakeLists.txt
#include <raylib.h>
#include <rlImGui.h>
#include <string>
#include <vector>

// Menu не залежить від Engine

// # ----------------------------------------------------------------------------
// # Menu - handles rendering and logic for main menu
// # ----------------------------------------------------------------------------

// class Engine; // Видалено

enum class MenuAction
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

enum class MenuState
{
    Main,
    GameMode,
    Options,
    Video,
    Audio,
    Controls,
    Credits,
    Mods,
    ConfirmExit
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
    ~Menu();

public:
    float Lerp(float a, float b, float t) const;
    [[nodiscard]] MenuAction GetAction() const;
    void RenderMenu() const;

public:
    void Update();
    void Render();
    void ResetAction();
    void RenderSettingsMenu();
    void RenderCredits();
    void RenderMods();
    void ExecuteAction();
    void HandleMouseSelection();
    void HandleKeyboardNavigation();
    void HandleVideoNavigation();
    void HandleConfirmExit();
    void RenderConfirmExit() const;
};

#endif
