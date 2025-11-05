#ifndef MENU_H
#define MENU_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <functional>
#include "SettingsManager.h"
#include "ConsoleManager.h"
#include "MapSelector.h"
#include "Engine/Engine.h"
#include <Render/IMenuRenderable.h>
#include <imgui.h>
#include <raylib.h>

// Forward declarations to break circular dependencies
class Kernel;


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
    OpenGameplay,
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
    ApplyControlSettings,
    ApplyGameplaySettings
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
    Gameplay,
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


struct VideoSettings
{
    int resolutionIndex = 1;
    int displayModeIndex = 0;
    int vsyncIndex = 1;
    int fpsIndex = 1;
};


class Menu : public IMenuRenderable
{
public:
    Menu();
    ~Menu() = default;

    // Core functionality
    void Initialize(Engine *engine);
    void SetKernel(Kernel* kernel);

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
    void ShowGameplayMenu();
    void ShowCredits();
    void ShowMods();
    void ShowConfirmExit();

    // Settings management
    void ApplyPendingSettings();
    void SaveConfiguration();
    void LoadConfiguration();

    // Map management
    [[nodiscard]] std::optional<MapInfo> GetSelectedMap() const;
    [[nodiscard]] std::string GetSelectedMapName() const;
    void InitializeMaps();

    // Action management
    void SetAction(MenuAction action);
    [[nodiscard]] MenuAction GetAction() const;
    void ResetAction();

    // Console functionality
    void ToggleConsole();
    [[nodiscard]] bool IsConsoleOpen() const;
    [[nodiscard]] ConsoleManager* GetConsoleManager() const;

    // Keyboard navigation
    void HandleKeyboardNavigation();
    void HandlePendingActions();

    // IMenuRenderable interface implementations
    void Update() override;
    void Render() override;

private:
    // ImGui rendering methods
    void BeginFrame();
    void EndFrame();
    void SetupStyle();
    void RenderMenuState();

    // Menu screen renderers
    void RenderMainMenu();
    void RenderOptionsMenu();
    void RenderGameModeMenu();
    void RenderMapSelection();
    void RenderAudioSettings();
    void RenderVideoSettings();
    void RenderControlSettings();
    void RenderGameplaySettings();
    void RenderCreditsScreen();
    void RenderModsScreen();
    void RenderConfirmExitDialog();

    // Console functionality
    void RenderConsoleOverlay();

    // Helper methods
    void HandleAction(MenuAction action);
    static const char *GetStateTitle(MenuState state) ;

    // ImGui UI components
    bool RenderActionButton(const char *label, MenuAction action, const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;
    void RenderMenuHint(const char *text) const;
    void RenderMapCard(int index, const MapInfo &map, bool selected, float cardWidth) const;
    
    // Video settings helpers
    bool RenderVideoSettingCombo(const char* label, const char* id, 
                                const std::vector<std::string>& options, 
                                int& currentIndex, float labelWidth, float comboWidth, float startX);
    bool HasUnsavedVideoChanges() const;

    // Pagination methods
    void EnsurePagination();
    void GoToNextPage();
    void GoToPreviousPage();
    [[nodiscard]] int GetPageStartIndex() const;
    [[nodiscard]] int GetPageEndIndex() const;
    [[nodiscard]] int GetTotalPages() const;
    void RenderPaginationControls();

    // Settings synchronization
    void SyncVideoSettingsToConfig();
    void SyncAudioSettingsToConfig();
    void SyncControlSettingsToConfig();
    void SyncGameplaySettingsToConfig();

    // Map management
    void ScanForJsonMaps();
    void UpdatePagination();

    // Core state
    Engine *m_engine = nullptr;
    Kernel* m_kernel = nullptr;
    std::unique_ptr<SettingsManager> m_settingsManager;

    // Menu state
    MenuState m_state = MenuState::Main;
    MenuAction m_pendingAction = MenuAction::None;
    MenuAction m_action = MenuAction::None;
    bool m_gameInProgress = false;

    // Settings
    VideoSettings m_videoSettings{};
    AudioSettings m_audioSettings{};
    ControlSettings m_controlSettings{};
    GameplaySettings m_gameplaySettings{};

    // Map management
    std::vector<MapInfo> m_availableMaps;
    int m_selectedMapIndex = 0;
    int m_mapsPerPage = 6;
    int m_currentPage = 0;
    int m_totalPages = 0;
    int m_jsonMapsCount = 0;

    // Navigation state
    int m_selected = 0;

    // Console manager
    std::unique_ptr<ConsoleManager> m_consoleManager;

    // Map selector
    std::unique_ptr<MapSelector> m_mapSelector;

    // UI state
    bool m_showDemoWindow = false;
    bool m_showStyleEditor = false;
    bool m_addResumeButton = false;
    ImGuiStyle m_customStyle{};

    // Options vectors
    std::vector<std::string> m_resolutionOptions;
    std::vector<std::string> m_displayModeOptions;
    std::vector<std::string> m_vsyncOptions;
    std::vector<std::string> m_fpsOptions;
    std::vector<std::string> m_difficultyOptions;
};

#endif // MENU_H
