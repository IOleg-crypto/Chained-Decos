//
// Created by Kilo Code
//

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "raylib.h"
#include <imgui.h>

#include "editor/Editor.h"
#include "editor/EditorTypes.h"
#include "editor/IEditor.h"
#include "editor/mapgui/skyboxBrowser.h"
#include "editor/tool/IToolManager.h"
#include "scene/resources/map/core/MapLoader.h"

// Configuration for UIManager
struct UIManagerConfig
{
    IEditor *editor = nullptr;
    int initialGridSize = 50;
};

// UI Layout Constants
static constexpr float MENU_BAR_HEIGHT = 19.0f;
static constexpr float PANEL_WIDTH = 300.0f;

// Concrete UI Manager implementation
class EditorUIManager : public IUIManager
{
private:
    // Subsystem references
    IEditor *m_editor;

    // UI state flags
    bool m_displayImGuiInterface;
    bool m_displayWelcomeScreen = true; // Default to true on startup
    bool m_pendingObjectCreation;
    bool m_displaySkyboxPanel;
    bool m_shouldExit = false; // Flag to signal application exit
    std::string m_currentlySelectedModelName;

    // Icons
    Texture2D m_iconNewProject;
    Texture2D m_iconOpenProject;
    bool m_iconsLoaded = false;

    // Save Prompt State
    enum class PendingAction : std::uint8_t
    {
        NONE,
        NEW_PROJECT,
        OPEN_PROJECT,
        LOAD_MAP
    };
    PendingAction m_pendingAction = PendingAction::NONE;
    bool m_showSavePrompt = false;

    // Parkour map dialog
    bool m_displayParkourMapDialog;
    std::vector<GameMap> m_availableParkourMaps;
    int m_currentlySelectedParkourMapIndex;

    // Skybox browser
    std::unique_ptr<SkyboxBrowser> m_skyboxBrowser;

public:
    explicit EditorUIManager(const UIManagerConfig &config);
    ~EditorUIManager() override;

    // IUIManager interface
    void Render() override;
    void HandleInput() override;
    void ShowObjectPanel(bool show) override;
    void ShowPropertiesPanel(bool show) override;
    int GetGridSize() const override;
    bool IsWelcomeScreenActive() const override
    {
        return m_displayWelcomeScreen;
    }
    void ToggleSkyboxBrowser() override
    {
        m_displaySkyboxPanel = !m_displaySkyboxPanel;
    }

    // UI state accessors
    bool IsImGuiInterfaceDisplayed() const
    {
        return m_displayImGuiInterface;
    }
    bool IsParkourMapDialogDisplayed() const
    {
        return m_displayParkourMapDialog;
    }

    // Tool and model state
    Tool GetActiveTool() const;
    void SetActiveTool(Tool tool);
    const std::string &GetSelectedModelName() const
    {
        return m_currentlySelectedModelName;
    }
    void SetSelectedModelName(const std::string &name)
    {
        m_currentlySelectedModelName = name;
    }
    void SetGridSize(int size);

    // Exit control
    bool ShouldExit() const
    {
        return m_shouldExit;
    }

private:
    // Rendering methods
    void RenderWelcomeScreen();
    void RenderImGuiToolbar();
    void RenderSavePrompt(); // Popup for unsaved changes

    // Input handling
    void HandleKeyboardInput();

    // UI helper methods
    void ProcessPendingObjectCreation()
    {
        if (m_pendingObjectCreation)
        {
            dynamic_cast<Editor *>(m_editor)->CreateDefaultObject(
                static_cast<MapObjectType>(GetActiveTool()), m_currentlySelectedModelName);
            m_pendingObjectCreation = false;
            SetActiveTool(SELECT);
        }
    }
    void ExecutePendingAction();

    // Window position helper (windowSize is passed by reference to allow clamping)
    ImVec2 ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize);

    // Ensure window stays within screen bounds (call after Begin())
    void EnsureWindowInBounds();
};

#endif // UIMANAGER_H
