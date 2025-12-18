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

#include "../ToolManager/IToolManager.h"
#include "IUIManager.h"
#include "scene/resources/map/Core/MapData.h"

class Editor;

// Configuration for UIManager
struct UIManagerConfig
{
    Editor *editor = nullptr;
    int initialGridSize = 50;
};

// Concrete UI Manager implementation
class EditorUIManager : public IUIManager
{
private:
    // Subsystem references
    Editor *m_editor;

    // UI state flags
    bool m_displayImGuiInterface;
    bool m_displayObjectListPanel;
    bool m_displayPropertiesPanel;
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
    std::unique_ptr<class SkyboxBrowser> m_skyboxBrowser;

public:
    explicit EditorUIManager(const UIManagerConfig &config);
    ~EditorUIManager() override;

    // IUIManager interface
    void Render() override;
    void HandleInput() override;
    void ShowObjectPanel(bool show) override;
    void ShowPropertiesPanel(bool show) override;
    int GetGridSize() const override;

    // UI state accessors
    bool IsImGuiInterfaceDisplayed() const
    {
        return m_displayImGuiInterface;
    }
    bool IsObjectPanelDisplayed() const
    {
        return m_displayObjectListPanel;
    }
    bool IsPropertiesPanelDisplayed() const
    {
        return m_displayPropertiesPanel;
    }
    bool IsParkourMapDialogDisplayed() const
    {
        return m_displayParkourMapDialog;
    }

    // Tool and model state
    ::Tool GetActiveTool() const;
    void SetActiveTool(::Tool tool);
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
    void RenderImGuiObjectPanel();
    void RenderImGuiPropertiesPanel();
    void RenderSavePrompt(); // Popup for unsaved changes

    // Input handling
    void HandleKeyboardInput();

    // UI helper methods
    void ProcessPendingObjectCreation();
    void ExecutePendingAction();

    // Object factory
    std::unique_ptr<class ObjectFactory> m_objectFactory;

    // Window position helper (windowSize is passed by reference to allow clamping)
    ImVec2 ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize);

    // Ensure window stays within screen bounds (call after Begin())
    void EnsureWindowInBounds();
};

#endif // UIMANAGER_H