//
// Created by Kilo Code
//

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "raylib.h"

#include "Engine/Map/MapLoader.h"
#include "../SceneManager/ISceneManager.h"
#include "../FileManager/IFileManager.h"
#include "../ToolManager/IToolManager.h"
#include "../ModelManager/IModelManager.h"
#include "IUIManager.h"

class Editor;


// Concrete UI Manager implementation
class UIManager : public IUIManager {
private:
    // Subsystem references
    Editor* m_editor;
    ISceneManager* m_sceneManager;
    IFileManager* m_fileManager;
    IToolManager* m_toolManager;
    IModelManager* m_modelManager;

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
    
    // Placeholder texture for skybox panel (avoid loading every frame)
    Texture2D m_skyboxPlaceholderTexture;
    bool m_skyboxPlaceholderInitialized;

    // Available skyboxes from resources/skyboxes/
    struct SkyboxInfo
    {
        std::string filename;
        std::string fullPath;
        Texture2D previewTexture;
        bool previewLoaded;
    };
    std::vector<SkyboxInfo> m_availableSkyboxes;
    bool m_skyboxesScanned;
    int m_selectedSkyboxIndex;

public:
    UIManager(Editor* editor,
              ISceneManager* sceneManager, IFileManager* fileManager,
              IToolManager* toolManager, IModelManager* modelManager);
    ~UIManager() override;

    // IUIManager interface
    void Render() override;
    void HandleInput() override;
    void ShowObjectPanel(bool show) override;
    void ShowPropertiesPanel(bool show) override;
    int GetGridSize() const override;

    // UI state accessors
    bool IsImGuiInterfaceDisplayed() const { return m_displayImGuiInterface; }
    bool IsObjectPanelDisplayed() const { return m_displayObjectListPanel; }
    bool IsPropertiesPanelDisplayed() const { return m_displayPropertiesPanel; }
    bool IsParkourMapDialogDisplayed() const { return m_displayParkourMapDialog; }

    // Tool and model state
    ::Tool GetActiveTool() const;
    void SetActiveTool(::Tool tool);
    const std::string& GetSelectedModelName() const { return m_currentlySelectedModelName; }
    void SetSelectedModelName(const std::string& name) { m_currentlySelectedModelName = name; }
    void SetGridSize(int size) { m_gridSizes = size; }

private:
    // Rendering methods
    void RenderImGuiToolbar();
    void RenderImGuiObjectPanel();
    void RenderImGuiPropertiesPanel();

    // Input handling
    void HandleKeyboardInput();

    // UI helper methods
    void ProcessPendingObjectCreation();
    
    // Window position helper (windowSize is passed by reference to allow clamping)
    ImVec2 ClampWindowPosition(const ImVec2& desiredPos, ImVec2& windowSize);
    
    // Ensure window stays within screen bounds (call after Begin())
    void EnsureWindowInBounds();
    // Render skybox panel
    void RenderSkyboxPanel();
    // Scan skyboxes directory
    void ScanSkyboxesDirectory();
    // Load preview for a skybox
    void LoadSkyboxPreview(SkyboxInfo& skybox);
};

#endif // UIMANAGER_H