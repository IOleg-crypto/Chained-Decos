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

#include "Game/Map/MapLoader.h"

// Forward declarations for subsystem interfaces
class ISceneManager;
class IFileManager;
class IToolManager;

// Include ModelManager interface
#include "../ModelManager/IModelManager.h"

#include "IUIManager.h"

// Include the Tool enum from ToolManager
#include "../ToolManager/IToolManager.h"

// Include necessary headers for complete types
#include "../SceneManager/ISceneManager.h"
#include "../FileManager/IFileManager.h"


// Concrete UI Manager implementation
class UIManager : public IUIManager {
private:
    // Subsystem references
    ISceneManager* m_sceneManager;
    IFileManager* m_fileManager;
    IToolManager* m_toolManager;
    IModelManager* m_modelManager;

    // UI state flags
    bool m_displayImGuiInterface;
    bool m_displayObjectListPanel;
    bool m_displayPropertiesPanel;
    bool m_pendingObjectCreation;
    std::string m_currentlySelectedModelName;
    int m_gridSizes;

    // File dialog state
    std::string m_currentWorkingDirectory;
    std::vector<std::string> m_currentDirectoryContents;
    std::string m_currentlySelectedFile;
    std::string m_newFileNameInput;
    std::string m_newFolderNameInput;
    bool m_displayFileDialog;
    bool m_isFileLoadDialog;
    bool m_isJsonExportDialog;
    bool m_displayNewFolderDialog;
    bool m_displayDeleteConfirmationDialog;
    std::string m_itemPendingDeletion;

    // Parkour map dialog
    bool m_displayParkourMapDialog;
    std::vector<GameMap> m_availableParkourMaps;
    int m_currentlySelectedParkourMapIndex;


public:
    UIManager(ISceneManager* sceneManager, IFileManager* fileManager,
              IToolManager* toolManager, IModelManager* modelManager);
    ~UIManager() override;

    // IUIManager interface
    void Render() override;
    void HandleInput() override;
    void ShowObjectPanel(bool show) override;
    void ShowPropertiesPanel(bool show) override;
    void ShowFileDialog(bool show) override;
    void ShowParkourMapDialog(bool show) override;

    // UI state accessors
    bool IsImGuiInterfaceDisplayed() const { return m_displayImGuiInterface; }
    bool IsObjectPanelDisplayed() const { return m_displayObjectListPanel; }
    bool IsPropertiesPanelDisplayed() const { return m_displayPropertiesPanel; }
    bool IsFileDialogDisplayed() const { return m_displayFileDialog; }
    bool IsParkourMapDialogDisplayed() const { return m_displayParkourMapDialog; }

    // Tool and model state
    ::Tool GetActiveTool() const;
    void SetActiveTool(::Tool tool);
    const std::string& GetSelectedModelName() const { return m_currentlySelectedModelName; }
    void SetSelectedModelName(const std::string& name) { m_currentlySelectedModelName = name; }
    int GetGridSize() const { return m_gridSizes; }
    void SetGridSize(int size) { m_gridSizes = size; }

private:
    // Rendering methods
    void RenderImGuiToolbar();
    void RenderImGuiObjectPanel();
    void RenderImGuiPropertiesPanel();
    void RenderParkourMapDialog();
    void RenderFileDialog();

    // Input handling
    void HandleKeyboardInput();

    // File dialog helpers
    void OpenFileDialog(bool isLoad);
    void RefreshDirectoryItems();
    void NavigateToDirectory(const std::string& path);

    // UI helper methods
    void ProcessPendingObjectCreation();
};

#endif // UIMANAGER_H