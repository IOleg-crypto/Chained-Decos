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
#include "Object/MapObject.h"
#include "Engine/Map/MapLoader.h"
#include "ModelManager/IModelManager.h"

// Subsystem interfaces
#include "SceneManager/ISceneManager.h"
#include "UIManager/IUIManager.h"
#include "FileManager/IFileManager.h"
#include "ToolManager/IToolManager.h"
#include "CameraManager/ICameraManager.h"

// Main editor class for the map editor - now acts as a facade
class Editor
{
private:
    // Subsystem managers (facade pattern)
    std::unique_ptr<ISceneManager> m_sceneManager;
    std::unique_ptr<IUIManager> m_uiManager;
    std::unique_ptr<IFileManager> m_fileManager;
    std::unique_ptr<IToolManager> m_toolManager;
    std::unique_ptr<ICameraManager> m_cameraManager;
    std::unique_ptr<IModelManager> m_modelManager;
    // TODO: Implement when needed
    // std::unique_ptr<IInputManager> m_inputManager;
    // std::unique_ptr<IRenderManager> m_renderManager;

    // Legacy compatibility - minimal state kept for backward compatibility
    int m_gridSizes;

public:
    Editor(std::shared_ptr<CameraController> cameraController, std::unique_ptr<ModelLoader> modelLoader);
    ~Editor();

public:
    // Core editor functions (facade interface)
    [[nodiscard]] std::shared_ptr<CameraController> GetCameraController() const;
    void Update();      // Update editor state
    void Render();      // Render 3D objects
    void RenderImGui(); // Render ImGui interface
    void HandleInput(); // Handle user input

    // Assets
    void PreloadModelsFromResources();

    // Object management functions (delegate to SceneManager)
    void AddObject(const MapObject &obj); // Add new object to scene
    void RemoveObject(int index);         // Remove object by index
    void SelectObject(int index);         // Select object by index
    void ClearSelection();                // Clear current selection

    // File operations (delegate to FileManager)
    void SaveMap(const std::string &filename); // Save map to file (editor format)
    void LoadMap(const std::string &filename); // Load map from file (editor format)
    void ExportMapForGame(const std::string &filename); // Export map for game engine
    void ExportMapAsJSON(const std::string &filename); // Export map as JSON format

    // Parkour map operations (delegate to FileManager)
    void LoadParkourMap(const std::string& mapName); // Load a parkour map into editor
    void GenerateParkourMap(const std::string& mapName); // Generate a new parkour map
    void ShowParkourMapSelector(); // Show parkour map selection dialog
    int GetGridSize() const; // Get editor grid size

    // Access to subsystems for advanced usage (optional - maintains some backward compatibility)
    ISceneManager* GetSceneManager() const { return m_sceneManager.get(); }
    IFileManager* GetFileManager() const { return m_fileManager.get(); }
    IToolManager* GetToolManager() const { return m_toolManager.get(); }
    ICameraManager* GetCameraManager() const { return m_cameraManager.get(); }
    IModelManager* GetModelManager() const { return m_modelManager.get(); }

private:
    // Helper methods for subsystem coordination
    void InitializeSubsystems(std::shared_ptr<CameraController> cameraController, std::unique_ptr<ModelLoader> modelLoader);
    void RenderObject(const MapObject& obj); // Render a single object
    void RenderGizmo(const MapObject& obj, const MapObjectData& data); // Render transform gizmo
};

#endif // EDITOR_H
