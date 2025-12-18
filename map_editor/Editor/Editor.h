//
// Created by I#Oleg.
//

#ifndef EDITOR_H
#define EDITOR_H

#include "raylib.h"
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "core/engine/Base.h"
#include "scene/3d/camera/Core/CameraController.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/map/Skybox/Skybox.h"
#include "scene/resources/model/Core/Model.h"

// Subsystem interfaces
#include "ToolManager/IToolManager.h"
#include "UIManager/IUIManager.h"

// Rendering and utilities
#include "Renderer/EditorRenderer.h"
#include "Utils/PathUtils.h"

// Main editor class for the map editor
class Editor
{
private:
    // Subsystem managers
    std::unique_ptr<IUIManager> m_uiManager;
    std::unique_ptr<IToolManager> m_toolManager;

    // Engine resources and services
    ChainedDecos::Ref<CameraController> m_cameraController;
    ChainedDecos::Ref<ModelLoader> m_modelLoader;
    std::unique_ptr<Skybox> m_skybox;
    GameMap m_gameMap; // The actual map data

    // State
    int m_gridSize = 50;
    int m_activeTool = 0;
    int m_selectedIndex = -1;
    bool m_isSceneModified = false;
    std::string m_currentMapPath;

    // Rendering helper
    std::unique_ptr<EditorRenderer> m_renderer;

    // Legacy/State kept for editor logic
    Texture2D m_spawnTexture;
    bool m_spawnTextureLoaded;
    Color m_clearColor;

public:
    Editor(ChainedDecos::Ref<CameraController> cameraController,
           ChainedDecos::Ref<ModelLoader> modelLoader);
    ~Editor();

public:
    // Core editor functions
    [[nodiscard]] ChainedDecos::Ref<CameraController> GetCameraController() const;
    void Update();      // Update editor state
    void Render();      // Render 3D objects
    void RenderImGui(); // Render ImGui interface
    void HandleInput(); // Handle user input

    // Assets
    void PreloadModelsFromResources();
    void LoadSpawnTexture(); // Load spawn zone texture

    // Object management (Moved from SceneManager)
    void AddObject(const MapObjectData &obj);
    void RemoveObject(int index);
    void SelectObject(int index);
    void ClearSelection();
    void ClearScene();

    // File operations (Moved from FileManager/using MapLoader)
    void SaveMap(const std::string &filename);
    void LoadMap(const std::string &filename);

    void SetGridSize(int size);
    int GetGridSize() const;

    // Tool Management
    int GetActiveTool() const;
    void SetActiveTool(int tool);

    // service accessors
    ChainedDecos::Ref<ModelLoader> GetModelLoader() const;

    // Skybox operations
    void ApplyMetadata(const MapMetadata &metadata);
    void SetSkyboxTexture(const std::string &texturePath);

    const std::string &GetSkyboxTexture() const
    {
        return m_gameMap.GetMapMetaData().skyboxTexture;
    }
    bool HasSkybox() const
    {
        return static_cast<bool>(m_skybox);
    }
    Skybox *GetSkybox() const
    {
        return m_skybox.get();
    }
    Color GetClearColor() const
    {
        return m_clearColor;
    }

    // Accessors for UI/Tools
    GameMap &GetGameMap()
    {
        return m_gameMap;
    }
    int GetSelectedObjectIndex() const
    {
        return m_selectedIndex;
    }
    MapObjectData *GetSelectedObject();

    IToolManager *GetToolManager() const
    {
        return m_toolManager.get();
    }
    IUIManager *GetUIManager() const
    {
        return m_uiManager.get();
    }

    bool IsSceneModified() const
    {
        return m_isSceneModified;
    }
    void SetSceneModified(bool modified)
    {
        m_isSceneModified = modified;
    }

    const std::string &GetCurrentMapPath() const
    {
        return m_currentMapPath;
    }

private:
    void InitializeSubsystems();
    void RenderObject(const MapObjectData &obj);

public:
    std::string GetSkyboxAbsolutePath() const;
};

#endif // EDITOR_H
