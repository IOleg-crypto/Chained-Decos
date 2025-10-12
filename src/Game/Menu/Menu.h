#ifndef MENU_H
#define MENU_H

#include <cstdint>
#include <string>
#include <vector>
#include <raylib.h>
#include <Config/ConfigManager.h>
#include <Engine/Engine.h>

enum class MenuAction : uint8_t
{
    None,
    StartGame,
    ResumeGame,        // New action to resume current game
    OpenOptions,
    OpenCredits,
    OpenVideoMode,
    OpenAudio,
    OpenControls,
    OpenGameplay,
    OpenParkourControls,
    OpenLanguage,
    OpenMods,
    BackToMainMenu,
    SinglePlayer,
    MultiPlayer,
    OpenGameModeMenu,
    ExitGame,
    StartGameWithMap,  // New action for starting game with selected map
    AdjustMasterVolume, // Audio control actions
    AdjustMusicVolume,
    AdjustSFXVolume,
    ToggleMute,
    OpenKeyBinding,     // Controls actions
    AdjustMouseSensitivity,
    ToggleInvertY,
    ToggleController,
    AdjustDifficulty,   // Gameplay actions
    ToggleTimer,
    ToggleCheckpoints,
    ToggleAutoSave,
    ToggleSpeedrunMode,
    AdjustWallRunSensitivity,  // Parkour control actions
    AdjustJumpTiming,
    AdjustSlideControl,
    AdjustGrappleSensitivity,
    ToggleWallRun,
    ToggleDoubleJump,
    ToggleSlide,
    ToggleGrapple,
    ToggleSlowMotionTrick
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
    Gameplay,
    ParkourControls,
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
    std::vector<MenuItem> m_gameplayMenu;
    std::vector<MenuItem> m_parkourControlsMenu;
    std::vector<MenuOption> m_videoOptions;
    std::vector<MenuOption> m_gameplayOptions;
    std::vector<MenuOption> m_parkourControlsOptions;

    // Map selection data
    std::vector<MapInfo> m_availableMaps;
    int m_selectedMap = 0;

    // Game state tracking
    bool m_gameInProgress = false;

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
    Font m_font; // Alan Sans font for menu text

    // Audio settings
    float m_masterVolume = 1.0f;
    float m_musicVolume = 0.7f;
    float m_sfxVolume = 0.8f;
    bool m_audioMuted = false;

    // Control settings
    float m_mouseSensitivity = 1.0f;
    bool m_invertYAxis = false;
    bool m_controllerSupport = true;

    // Parkour-specific control settings
    float m_wallRunSensitivity = 1.0f;
    float m_jumpTiming = 1.0f;
    float m_slideControl = 1.0f;
    float m_grappleSensitivity = 1.0f;

    // Gameplay settings
    int m_difficultyLevel = 2;
    bool m_timerEnabled = true;
    bool m_checkpointsEnabled = true;
    bool m_autoSaveEnabled = true;
    bool m_speedrunMode = false;

    // Advanced parkour settings
    bool m_wallRunEnabled = true;
    bool m_doubleJumpEnabled = false;
    bool m_slideEnabled = true;
    bool m_grappleEnabled = false;
    bool m_slowMotionOnTrick = false;

public:
    Menu();

    float Lerp(float a, float b, float t) const;
    [[nodiscard]] MenuAction GetAction() const;
    void RenderMenu() const;

    void Update();
    void Render();
    void GetEngine(Engine *engine);
    void ResetAction();
    void RenderSettingsMenu() const;
    void RenderCredits();
    void RenderMods();
    void ExecuteAction();
    void HandleMouseSelection();
    void HandleKeyboardNavigation();
    void HandleVideoNavigation();
    void HandleConfirmExit();
    void HandleMapSelection();
    void RenderConfirmExit();
    void SetAction(MenuAction type);

    // Map selection methods
    void InitializeMaps();
    void RenderMapSelection() const;
    [[nodiscard]] const MapInfo* GetSelectedMap() const;
    [[nodiscard]] std::string GetSelectedMapName() const;
    void ScanForJsonMaps();

    // Helper method for dynamic menu items
    [[nodiscard]] const std::vector<MenuItem>* GetCurrentMenuWithDynamicItems() const;

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

    // Game state management
    void SetGameInProgress(bool inProgress);
    [[nodiscard]] bool IsGameInProgress() const { return m_gameInProgress; }

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
    void HandleGameplayMouseSelection(Vector2 mousePos, bool clicked);
    void HandleParkourControlsMouseSelection(Vector2 mousePos, bool clicked);

    // New navigation and rendering functions for options-based menus
    void HandleGameplayNavigation();
    void HandleParkourControlsNavigation();
    void RenderGameplayMenu();
    void RenderParkourControlsMenu();
    void ApplyGameplayOption(MenuOption& opt);
    void ApplyParkourControlsOption(MenuOption& opt);
    std::string GetGameplaySettingValue(const std::string& settingName) const;
    std::string GetParkourControlsSettingValue(const std::string& settingName) const;

    // Test accessor method
    [[nodiscard]] bool IsVisible() const { return m_state != MenuState::Main || m_gameInProgress; }
};

#endif // MENU_H
