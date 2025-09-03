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


#include "Engine/CameraController/CameraController.h" // Коректний шлях
#include "Engine/Model/Model.h"                       // Коректний шлях
#include "MapObject.h"

// Main editor class for the map editor
class Editor
{
private:
    std::shared_ptr<CameraController> m_cameraController; // Camera controller for 3D view
    std::vector<MapObject> m_objects;                     // List of all objects in the scene
    int m_selectedObjectIndex;                            // Index of currently selected object
    int m_currentTool;                                    // Current editing tool
    bool m_showImGui;                                     // Show/hide ImGui interface
    bool m_showObjectPanel;                               // Show/hide object list panel
    bool m_showPropertiesPanel;                           // Show/hide properties panel
    bool m_shouldAddObject;                               // Add cube , cylinder or sphere
    std::string m_mapFileName;                            // Current map file name
    ModelLoader m_models;                            // Model manager for loading and rendering models
    std::vector<std::string> m_availableModels; // List of available models
    std::string m_selectedModelName;            // Currently selected model for adding
    bool m_modelsInitialized;                   // Flag to track if models are loaded

    // File dialog
    bool m_showFileDialog;                     // Show/hide file dialog
    bool m_isLoadDialog;                       // true for Load, false for Save
    std::string m_currentDirectory;            // Current directory in file browser
    std::vector<std::string> m_directoryItems; // Items in current directory
    std::string m_selectedFile;                // Currently selected file
    std::string m_newFileName;                 // New file name for save dialog
    std::string m_newFolderName;               // New folder name for file dialog
    bool m_showNewFolderDialog;                // Show/hide new folder dialog
    bool m_showDeleteDialog;                   // Show/hide delete confirmation dialog
    std::string m_itemToDelete;                // Item to delete

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
    Editor();
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
    void SaveMap(const std::string &filename); // Save map to file
    void LoadMap(const std::string &filename); // Load map from file

private:
    // Rendering functions
    void RenderObject(MapObject &obj); // Render single object
    void RenderImGuiObjectPanel();     // Render object list panel
    void RenderImGuiPropertiesPanel(); // Render properties panel
    void RenderImGuiToolbar();         // Render toolbar

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
    void AddFolder();                                   // Add folder in file dialog
    void DeleteFolder(const std::string &selectedItem); // Delete selected file or folder
    void RenderDeleteConfirmDialog();                   // Render delete confirmation dialog
    void RenderNewFolderDialog();                       // Render new folder dialog
};

#endif // EDITOR_H
