#ifndef MENU_H
#define MENU_H

#include "Console/ConsoleManager.h"
#include "MapSelector/MapSelector.h"
#include "MenuPresenter.h"
#include "Settings/MenuSettingsController.h"
#include "Settings/SettingsManager.h"

#include "core/interfaces/IMenu.h"
#include "scene/3d/camera/Interfaces/ICameraSensitivityController.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <imgui.h>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <vector>

enum class MenuAction : uint8_t
{
    None,
    StartGame,
    ResumeGame,
    OpenOptions,
    OpenCredits,
    OpenVideoMode,
    OpenAudio,
    OpenControls,
    OpenMods,
    BackToMainMenu,
    SinglePlayer,
    MultiPlayer,
    ExitGame,
    StartGameWithMap,
    ToggleMute,
    OpenKeyBinding,
    ToggleInvertY,
    ToggleController,
    ApplyVideoSettings,
    ApplyAudioSettings,
    ApplyControlSettings
};

enum class MenuState : uint8_t
{
    Main,
    GameMode,
    Resume,
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
    std::string label;
    MenuAction action;
    bool enabled = true;
    std::string shortcut;
};
class Engine; // Forward declaration

class Menu : public IMenu
{
public:
    Menu();
    ~Menu() = default;

    // Core functionality
    void Initialize(Engine *engine);

    // State management
    void SetGameInProgress(bool inProgress);
    [[nodiscard]] bool IsGameInProgress() const;
    [[nodiscard]] MenuAction ConsumeAction();
    [[nodiscard]] MenuState GetState() const;
    void SetState(MenuState state);
    void SetResumeButtonOn(bool status);
    bool GetResumeButtonStatus() const;

    // Navigation methods
    void ShowMainMenu();
    void ShowOptionsMenu();
    void ShowGameModeMenu();
    void ShowMapSelection();
    void ShowAudioMenu();
    void ShowVideoMenu();
    void ShowControlsMenu();
    void ShowCredits();
    void ShowMods();
    void ShowConfirmExit();

    // Settings management
    void ApplyPendingSettings();
    void SaveConfiguration();
    void LoadConfiguration();

    // Map management
    [[nodiscard]] std::optional<MapInfo> GetSelectedMap() const;
    std::string GetSelectedMapName() const override;
    void InitializeMaps();

    // Action management
    void SetAction(MenuAction action);
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();

    // Console functionality
    void ToggleConsole() override;
    bool IsConsoleOpen() const override;
    [[nodiscard]] ConsoleManager *GetConsoleManager() const;

    // Settings manager access
    [[nodiscard]] SettingsManager *GetSettingsManager() const;

    // Dependency Injection for camera
    void SetCameraController(ICameraSensitivityController *controller);

    // Keyboard navigation
    void HandleKeyboardNavigation();
    void HandlePendingActions();

    // IMenu Interface Implementation
    bool IsOpen() const override;
    void Show() override;
    void Hide() override;
    bool ShouldStartGame() const override;

    // IMenu interface implementations
    void Update() override;
    void Render() override;

    // Public for initialization
    void SetupStyle();

private:
    // ImGui rendering methods
    void BeginFrame();
    void EndFrame();
    void RenderMenuState();

    // Menu screen renderers
    void RenderMainMenu();
    void RenderOptionsMenu();
    void RenderGameModeMenu();
    void RenderMapSelection();
    void RenderCreditsScreen();
    void RenderModsScreen();
    void RenderConfirmExitDialog();

    // Console functionality
    void RenderConsoleOverlay();

    // Helper methods
    void HandleAction(MenuAction action);
    static const char *GetStateTitle(MenuState state);

    // ImGui UI components
    bool RenderActionButton(const char *label, MenuAction action,
                            const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;
    void RenderMenuHint(const char *text) const;

    // Core state
    Engine *m_engine = nullptr;
    std::unique_ptr<SettingsManager> m_settingsManager;

    // Explicit dependency via interface (Dependency Injection)
    ICameraSensitivityController *m_cameraController = nullptr;

    // Menu state
    MenuState m_state = MenuState::Main;
    MenuAction m_pendingAction = MenuAction::None;
    MenuAction m_action = MenuAction::None;
    bool m_gameInProgress = false;

    // Navigation state
    int m_selected = 0;

    // Console manager
    std::unique_ptr<ConsoleManager> m_consoleManager;

    // Map selector
    std::unique_ptr<MapSelector> m_mapSelector;

    // Settings controller
    std::unique_ptr<MenuSettingsController> m_settingsController;

    // Menu presenter
    std::unique_ptr<MenuPresenter> m_presenter;

    // UI state
    bool m_showDemoWindow = false;
    bool m_showStyleEditor = false;
    bool m_addResumeButton = false;
    ImGuiStyle m_customStyle{};
};

#endif // MENU_H
