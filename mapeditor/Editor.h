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

#include "core/events/Event.h"
#include "core/utils/Base.h"
#include "mapeditor/IEditor.h"
#include "mapeditor/mapgui/IUIManager.h"
#include "mapeditor/tool/IToolManager.h"
#include "scene/camera/core/CameraController.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/map/skybox/skybox.h"
#include "scene/resources/model/core/Model.h"

// Rendering and utilities
#include "mapeditor/render/EditorRenderer.h"

#include "mapeditor/logic/MapManager.h"

// Main editor class for the map editor
class Editor : public IEditor
{
private:
    // Subsystem managers
    std::unique_ptr<IUIManager> m_uiManager;
    std::unique_ptr<IToolManager> m_toolManager;
    std::unique_ptr<MapManager> m_mapManager;

    // Engine resources and services
    ChainedDecos::Ref<CameraController> m_cameraController;
    ChainedDecos::Ref<IModelLoader> m_modelLoader;
    std::unique_ptr<Skybox> m_skybox;

    // State
    int m_gridSize = 50;
    int m_activeTool = 0;

    // Rendering helper
    std::unique_ptr<EditorRenderer> m_renderer;

    // Legacy/State kept for editor logic
    Texture2D m_spawnTexture;
    bool m_spawnTextureLoaded;
    Color m_clearColor;

public:
    Editor(ChainedDecos::Ref<CameraController> cameraController,
           ChainedDecos::Ref<IModelLoader> modelLoader);
    ~Editor();

public:
    // Core editor functions
    CameraController &GetCameraController() override;
    void Update();      // Update editor state
    void Render();      // Render 3D objects
    void RenderImGui(); // Render ImGui interface
    void HandleInput(); // Handle user input

    // Event handling
    void OnEvent(ChainedDecos::Event &e);

    // Grid settings
    void PreloadModelsFromResources();
    void LoadSpawnTexture(); // Load spawn zone texture

    // Object management (Moved from SceneManager)
    void AddObject(const MapObjectData &obj) override;
    void RemoveObject(int index) override;
    void SelectObject(int index) override;
    void ClearSelection() override;
    void ClearObjects() override;
    void ClearScene(); // Keep legacy name if used elsewhere
    void CreateDefaultObject(MapObjectType type, const std::string &modelName = "");

    // File operations (Moved from FileManager/using MapLoader)
    void SaveMap(const std::string &filename) override;
    void LoadMap(const std::string &filename) override;

    void SetGridSize(int size) override;
    int GetGridSize() const override;

    // Tool Management
    Tool GetActiveTool() const override;
    void SetActiveTool(Tool tool) override;

    // service accessors
    ChainedDecos::Ref<IModelLoader> GetModelLoader() override;

    // Skybox operations
    void ApplyMetadata(const MapMetadata &metadata);
    void SetSkybox(const std::string &name) override;
    void SetSkyboxTexture(const std::string &texturePath) override;
    void SetSkyboxColor(Color color) override;

    const std::string &GetSkyboxTexture() const;
    bool HasSkybox() const;
    Skybox *GetSkybox() const override;
    Color GetClearColor() const;

    // Accessors for UI/Tools
    GameMap &GetGameMap() override;
    int GetSelectedObjectIndex() const override;
    MapObjectData *GetSelectedObject() override;

    IToolManager *GetToolManager() const;
    IUIManager *GetUIManager() const;

    const std::string &GetCurrentMapPath() const override;

    bool IsSceneModified() const override;
    void SetSceneModified(bool modified) override;

private:
    void InitializeSubsystems();
    void RenderObject(const MapObjectData &obj);

public:
    std::string GetSkyboxAbsolutePath() const;
};

#endif // EDITOR_H


