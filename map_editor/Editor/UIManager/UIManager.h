//
// Created by Kilo Code
//

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <memory>
#include <string>
#include <vector>

#include "raylib.h"
#include <imgui.h>

#include "../FileManager/IFileManager.h"
#include "../ModelManager/IModelManager.h"
#include "../SceneManager/ISceneManager.h"
#include "../ToolManager/IToolManager.h"
#include "IUIManager.h"
#include "scene/resources/map/Core/MapLoader.h"

class Editor;

// Configuration for UIManager
struct UIManagerConfig
{
    Editor *editor = nullptr;
    ISceneManager *sceneManager = nullptr;
    IFileManager *fileManager = nullptr;
    IToolManager *toolManager = nullptr;
    IModelManager *modelManager = nullptr;
    int initialGridSize = 900;
};

// Concrete UI Manager implementation
class EditorUIManager : public IUIManager
{
private:
    // Subsystem references
    Editor *m_editor;
    ISceneManager *m_sceneManager;
    IFileManager *m_fileManager;
    IToolManager *m_toolManager;
    IModelManager *m_modelManager;

    // UI state flags
    bool m_displayImGuiInterface;
    bool m_displayObjectListPanel;
    bool m_displayPropertiesPanel;
    bool m_pendingObjectCreation;
    bool m_displaySkyboxPanel;
    std::string m_currentlySelectedModelName;
    int m_gridSizes;

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
    void SetGridSize(int size)
    {
        m_gridSizes = size;
    }

private:
    // Rendering methods
    void RenderImGuiToolbar();
    void RenderImGuiObjectPanel();
    void RenderImGuiPropertiesPanel();

    // Input handling
    void HandleKeyboardInput();

    // UI helper methods
    void ProcessPendingObjectCreation();

    // Object factory
    std::unique_ptr<class ObjectFactory> m_objectFactory;

    // Window position helper (windowSize is passed by reference to allow clamping)
    ImVec2 ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize);

    // Ensure window stays within screen bounds (call after Begin())
    void EnsureWindowInBounds();
};

#endif // UIMANAGER_H