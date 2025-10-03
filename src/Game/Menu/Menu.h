#ifndef MENU_H
#define MENU_H

#include <cstdint>
#include <string>
#include <vector>
#include <raylib.h>
#include <Config/ConfigManager.h>

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
    ExitGame,
    StartGameWithMap,  // New action for starting game with selected map
    SelectMap1,        // Map selection actions
    SelectMap2,
    SelectMap3
};

enum class MenuState : uint8_t
{
    Main,
    GameMode,
    MapSelection,
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
    int selectedIndex = 0;
};

struct MapInfo
{
    std::string name;
    std::string displayName;
    std::string description;
    std::string previewImage; // Path to preview image
    Color themeColor;
    bool isAvailable;
};

class Menu
{
private:
    int m_selected = 0;
    MenuAction m_action = MenuAction::None;
    MenuState m_state = MenuState::Main;
    mutable std::vector<float> m_buttonScales;

    const std::vector<MenuItem> *m_currentMenu = nullptr;

    std::vector<MenuItem> m_mainMenu;
    std::vector<MenuItem> m_optionsMenu;
    std::vector<MenuItem> m_SetGameMode;
    std::vector<MenuItem> m_audioMenu;
    std::vector<MenuItem> m_controlsMenu;
    std::vector<MenuOption> m_videoOptions;

    // Map selection data
    std::vector<MapInfo> m_availableMaps;
    int m_selectedMap = 0;

    // Console functionality
    bool m_consoleOpen = false;
    std::string m_consoleInput;
    std::vector<std::string> m_consoleHistory;
    std::vector<std::string> m_consoleOutput;
    size_t m_consoleHistoryIndex = 0;
    static const size_t MAX_CONSOLE_LINES = 100;
    static const size_t MAX_HISTORY_LINES = 50;

    Engine *m_engine = nullptr;
    ConfigManager m_config;

public:
    Menu();

    float Lerp(float a, float b, float t) const;
    [[nodiscard]] MenuAction GetAction() const;
    void RenderMenu() const;

    void Update();
    void Render() const;
    void GetEngine(Engine *engine);
    void ResetAction();
    void RenderSettingsMenu() const;
    static void RenderCredits();
    static void RenderMods();
    void ExecuteAction();
    void HandleMouseSelection();
    void HandleKeyboardNavigation();
    void HandleVideoNavigation();
    void HandleConfirmExit();
    void HandleMapSelection();
    static void RenderConfirmExit();

    // Map selection methods
    void InitializeMaps();
    void RenderMapSelection() const;
    [[nodiscard]] const MapInfo* GetSelectedMap() const;
    [[nodiscard]] std::string GetSelectedMapName() const;

    // Console functionality
    void ToggleConsole();
    void HandleConsoleInput();
    void RenderConsole() const;
    void ExecuteConsoleCommand(const std::string& command);
    void AddConsoleOutput(const std::string& text);
    [[nodiscard]] bool IsConsoleOpen() const { return m_consoleOpen; }
    [[nodiscard]] std::string GetCurrentSettingValue(const std::string& settingName) const;

    // Configuration management
    void LoadSettings();
    void SaveSettings();

    // Keyboard navigation handlers
    void HandleMainMenuKeyboardNavigation();
    void HandleVideoMenuKeyboardNavigation();
    void HandleSimpleScreenKeyboardNavigation();
    void HandleMapSelectionKeyboardNavigation();
    void HandleConfirmExitKeyboardNavigation();

    // Mouse selection handlers
    void HandleMainMenuMouseSelection(Vector2 mousePos, bool clicked);
    void HandleVideoMenuMouseSelection(Vector2 mousePos, bool clicked);
    void HandleCreditsMouseSelection(Vector2 mousePos, bool clicked);
    void HandleModsMouseSelection(Vector2 mousePos, bool clicked);
    void HandleMapSelectionMouseSelection(Vector2 mousePos, bool clicked);
    void HandleConfirmExitMouseSelection(Vector2 mousePos, bool clicked);
};

#endif // MENU_H
