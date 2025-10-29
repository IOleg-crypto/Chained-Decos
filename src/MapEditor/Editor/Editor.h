//
// Created by I#Oleg.
//

#ifndef EDITOR_H
#define EDITOR_H

#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

#include "Engine/CameraController/CameraController.h"
#include "Engine/Model/Model.h"
#include "MapObject.h"
#include "../Map/MapLoader.h"
// Main editor class for the map editor
class Editor
{
private:
    std::shared_ptr<CameraController> m_cameraController;           // Camera controller for 3D view
    std::vector<MapObject> m_editorSceneObjects;                   // List of all objects in the scene
    int m_currentlySelectedObjectIndex;                             // Index of currently selected object
    int m_activeEditorTool;                                         // Current editing tool
    bool m_displayImGuiInterface;                                   // Show/hide ImGui interface
    bool m_displayObjectListPanel;                                   // Show/hide object list panel
    bool m_displayPropertiesPanel;                                  // Show/hide properties panel
    bool m_pendingObjectCreation;                                   // Flag for pending object creation
    std::string m_currentlyLoadedMapFilePath;                      // Current map file path
    std::unique_ptr<ModelLoader> m_modelAssetManager;               // Model manager for loading and rendering models
    std::vector<std::string> m_availableModelNamesList;             // List of available models
    std::vector<ModelInfo> m_availableModels;                       // Detailed model information with categories
    std::string m_currentlySelectedModelName;                       // Currently selected model for adding
    bool m_modelsInitialized;                                       // Flag to track if models are loaded
    int m_gridSizes;


    // File dialog
    bool m_displayFileDialog;                                     // Show/hide file dialog
    bool m_isFileLoadDialog;                                      // true for Load, false for Save
    bool m_isJsonExportDialog;                                    // true for JSON export
    std::string m_currentWorkingDirectory;                        // Current directory in file browser
    std::vector<std::string> m_currentDirectoryContents;          // Items in current directory
    std::string m_currentlySelectedFile;                          // Currently selected file
    std::string m_newFileNameInput;                               // New file name for save dialog
    std::string m_newFolderNameInput;                             // New folder name for file dialog
    bool m_displayNewFolderDialog;                                // Show/hide new folder dialog
    bool m_displayDeleteConfirmationDialog;                       // Show/hide delete confirmation dialog
    std::string m_itemPendingDeletion;                            // Item to delete

    // Parkour map dialog
    bool m_displayParkourMapDialog;                               // Show/hide parkour map dialog
    std::vector<GameMap> m_availableParkourMaps;                  // Available parkour maps
    int m_currentlySelectedParkourMapIndex;                       // Index of selected parkour map

    // Available editing tools
    enum Tool
    {
        SELECT = 0,       // Select objects
        MOVE = 1,         // Move objects
        ROTATE = 2,       // Rotate objects
        SCALE = 3,        // Scale objects
        ADD_CUBE = 4,     // Add cube primitive
        ADD_SPHERE = 5,   // Add sphere primitive
        ADD_CYLINDER = 6, // Add cylinder primitive
        ADD_MODEL = 7     // Add 3D model
    };

public:
    Editor(std::shared_ptr<CameraController> cameraController, std::unique_ptr<ModelLoader> modelLoader);
    ~Editor();

public:
    // Core editor functions
    [[nodiscard]] std::shared_ptr<CameraController> GetCameraController() const;
    void Update();      // Update editor state
    void Render();      // Render 3D objects
    void RenderImGui(); // Render ImGui interface
    void HandleInput(); // Handle user input

    // Object management functions
    void AddObject(const MapObject &obj); // Add new object to scene
    void RemoveObject(int index);         // Remove object by index
    void SelectObject(int index);         // Select object by index
    void ClearSelection();                // Clear current selection

    // File operations
    void SaveMap(const std::string &filename); // Save map to file (editor format)
    void LoadMap(const std::string &filename); // Load map from file (editor format)
    void ExportMapForGame(const std::string &filename); // Export map for game engine
    void ExportMapAsJSON(const std::string &filename); // Export map as JSON format

    // Parkour map operations
    void LoadParkourMap(const std::string& mapName); // Load a parkour map into editor
    void GenerateParkourMap(const std::string& mapName); // Generate a new parkour map
    void ShowParkourMapSelector(); // Show parkour map selection dialog
    int GetGridSize() const; // Get editor grid size

private:
    // Rendering functions
    void RenderObject(MapObject &obj); // Render single object
    void RenderImGuiObjectPanel();     // Render object list panel
    void RenderImGuiPropertiesPanel(); // Render properties panel
    void RenderImGuiToolbar();         // Render toolbar
    void RenderParkourMapDialog();     // Render parkour map selection dialog

    // Input handling
    void PickObject();          // Handle mouse input
    void HandleKeyboardInput(); // Handle keyboard input

    // Model management
    void EnsureModelsLoaded();                         // Ensure models are loaded
    Model *GetModelSafe(const std::string &modelName); // Safe model retrieval

    // File dialog
    void OpenFileDialog(bool isLoad);                   // Open file dialog
    void RenderFileDialog();                            // Render file dialog
    void RefreshDirectoryItems();                       // Refresh current directory items
    void NavigateToDirectory(const std::string &path);  // Navigate to directory
    
};

#endif // EDITOR_H
