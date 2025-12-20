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
#include "editor/IEditor.h"
#include "editor/mapgui/IUIManager.h"
#include "editor/tool/IToolManager.h"
#include "scene/camera/core/CameraController.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/map/skybox/skybox.h"
#include "scene/resources/model/core/Model.h"

// Rendering and utilities
#include "editor/render/EditorRenderer.h"

#include "editor/logic/MapManager.h"
#include "editor/panels/EditorPanelManager.h"

// Main editor class for ChainedEditor
class Editor : public IEditor
{
private:
    // Subsystem managers
    std::unique_ptr<IUIManager> m_uiManager;
    std::unique_ptr<IToolManager> m_toolManager;
    std::unique_ptr<MapManager> m_mapManager;
    std::unique_ptr<EditorPanelManager> m_panelManager;

    // Engine resources and services
    ChainedDecos::Ref<CameraController> m_cameraController;
    ChainedDecos::Ref<IModelLoader> m_modelLoader;
    std::unique_ptr<Skybox> m_skybox;

    // State
    int m_gridSize = 9999999;
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
    void CreateDefaultObject(MapObjectType type, const std::string &modelName = "") override;
    void LoadAndSpawnModel(const std::string &path) override;

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
    void ApplyMetadata(const MapMetadata &metadata) override;
    void SetSkybox(const std::string &name) override;
    void SetSkyboxTexture(const std::string &texturePath) override;
    void SetSkyboxColor(Color color) override;

    const std::string &GetSkyboxTexture() const;
    bool HasSkybox() const;
    Skybox *GetSkybox() const override;
    Color GetClearColor() const override;

    // Accessors for UI/Tools
    GameMap &GetGameMap() override;
    int GetSelectedObjectIndex() const override;
    MapObjectData *GetSelectedObject() override;

    IToolManager *GetToolManager() const;
    IUIManager *GetUIManager() const override;
    EditorPanelManager *GetPanelManager() const override
    {
        return m_panelManager.get();
    }

    const std::string &GetCurrentMapPath() const override;

    bool IsSceneModified() const override;
    void SetSceneModified(bool modified) override;

    // Play Mode Management
    void StartPlayMode() override;
    void StopPlayMode() override;
    bool IsInPlayMode() const override;

private:
    void InitializeSubsystems();
    void RenderObject(const MapObjectData &obj);

    void BuildGame() override;

    // Debug Visualization
    bool IsWireframeEnabled() const override
    {
        return m_drawWireframe;
    }
    void SetWireframeEnabled(bool enabled) override
    {
        m_drawWireframe = enabled;
    }
    bool IsCollisionDebugEnabled() const override
    {
        return m_drawCollisions;
    }
    void SetCollisionDebugEnabled(bool enabled) override
    {
        m_drawCollisions = enabled;
    }

private:
    bool m_drawWireframe = false;
    bool m_drawCollisions = false;
    bool m_isInPlayMode = false;

public:
    std::string GetSkyboxAbsolutePath() const;
};

#endif // EDITOR_H
