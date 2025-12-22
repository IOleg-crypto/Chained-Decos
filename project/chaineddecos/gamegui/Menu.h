#ifndef MENU_H
#define MENU_H

#include "console/consoleManager.h"
#include "interfaces/IMenuScreen.h"
#include "mapselector/mapselector.h"
#include "settings/MenuSettingsController.h"
#include "settings/settingsManager.h"


#include "events/MenuEvent.h"
#include "core/interfaces/IMenu.h"
#include "scene/camera/core/ICameraSensitivityController.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <imgui.h>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <vector>

#include "core/Engine.h"

enum class MenuState : uint8_t
{
    Main,
    Resume,
    MapSelection,
    Options,
    Video,
    Audio,
    Controls,
    Credits,
    ConfirmExit
};

class Menu : public IMenu
{
public:
    Menu();
    ~Menu() = default;

    // Core functionality
    void Initialize(ChainedEngine::Engine *engine);

    // Event management
    using MenuEventCallback = std::function<void(const ChainedDecos::MenuEvent &)>;
    void SetEventCallback(const MenuEventCallback &callback);

    // State management
    void SetGameInProgress(bool inProgress);
    [[nodiscard]] bool IsGameInProgress() const;
    [[nodiscard]] MenuState GetState() const;
    void SetState(MenuState state);
    void SetResumeButtonOn(bool status);
    bool GetResumeButtonStatus() const;

    // Screen management (State Pattern)
    void SetScreen(std::unique_ptr<IMenuScreen> screen);
    void ShowMainMenu();
    void ShowOptionsMenu();
    void ShowMapSelection();
    void ShowAudioMenu();
    void ShowVideoMenu();
    void ShowControlsMenu();
    void ShowCredits();
    void ShowConfirmExit();

    // Settings management
    void ApplyPendingSettings();
    void SaveConfiguration();
    void LoadConfiguration();

    // Map management
    [[nodiscard]] std::optional<MapInfo> GetSelectedMap() const;
    std::string GetSelectedMapName() const override;
    void InitializeMaps();

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

    void Update() override;
    void Render() override;

    // Public for initialization and screens
    void SetupStyle();

    // UI Helpers (Called by screens)
    bool RenderActionButton(const char *label, ChainedDecos::MenuEventType eventType,
                            const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;
    void RenderMenuHint(const char *text) const;

    // Internal getters for screens
    MapSelector *GetMapSelector() const
    {
        return m_mapSelector.get();
    }
    MenuSettingsController *GetSettingsController() const
    {
        return m_settingsController.get();
    }
    void DispatchEvent(ChainedDecos::MenuEventType type, const std::string &data = "");

private:
    void RenderConsoleOverlay();

    // Core state
    ChainedEngine::Engine *m_engine = nullptr;
    std::unique_ptr<SettingsManager> m_settingsManager;
    ICameraSensitivityController *m_cameraController = nullptr;

    // Menu state
    MenuState m_state = MenuState::Main;
    std::unique_ptr<IMenuScreen> m_currentScreen;
    MenuEventCallback m_eventCallback;
    bool m_gameInProgress = false;

    // Navigation state
    int m_selected = 0;

    // Managers
    std::unique_ptr<ConsoleManager> m_consoleManager;
    std::unique_ptr<MapSelector> m_mapSelector;
    std::unique_ptr<MenuSettingsController> m_settingsController;

    // UI state
    bool m_showDemoWindow = false;
    bool m_showStyleEditor = false;
    bool m_addResumeButton = false;
    ImGuiStyle m_customStyle{};
};

#endif // MENU_H
