#ifndef IEDITOR_H
#define IEDITOR_H

#include <core/utils/Base.h>
#include <string>

#include "editor/EditorTypes.h"
#include <scene/camera/core/CameraController.h>
#include <scene/resources/map/core/SceneLoader.h>
#include <scene/resources/map/skybox/skybox.h>
#include <scene/resources/model/core/Model.h>

class IEditor
{
public:
    virtual ~IEditor() = default;

    // Object Management
    virtual MapObjectData *GetSelectedObject() = 0;
    virtual int GetSelectedObjectIndex() const = 0;
    virtual void AddObject(const MapObjectData &obj) = 0;
    virtual void RemoveObject(int index) = 0;
    virtual void SelectObject(int index) = 0;
    virtual void ClearSelection() = 0;
    virtual void ClearObjects() = 0;
    virtual void ClearScene() = 0;

    // UI Selection
    virtual void SelectUIElement(int index) = 0;
    virtual int GetSelectedUIElementIndex() const = 0;
    virtual void RefreshUIEntities() = 0;

    // Map and Scene State
    virtual bool IsSceneModified() const = 0;
    virtual void SetSceneModified(bool modified) = 0;
    virtual const std::string &GetCurrentMapPath() const = 0;
    virtual void SaveScene(const std::string &path = "") = 0;
    virtual void LoadScene(const std::string &path) = 0;
    virtual GameScene &GetGameScene() = 0;

    // Tools and Grid
    virtual Tool GetActiveTool() const = 0;
    virtual void SetActiveTool(Tool tool) = 0;
    virtual int GetGridSize() const = 0;
    virtual void SetGridSize(int size) = 0;
    virtual void CreateDefaultObject(MapObjectType type, const std::string &modelName = "") = 0;
    virtual void LoadAndSpawnModel(const std::string &path) = 0;
    virtual void ApplyMetadata(const MapMetadata &metadata) = 0;

    // Service Accessors
    virtual ChainedDecos::Ref<IModelLoader> GetModelLoader() = 0;
    virtual CameraController &GetCameraController() = 0;

    // Skybox Operations
    virtual void SetSkybox(const std::string &name) = 0;
    virtual void SetSkyboxTexture(const std::string &texturePath) = 0;
    virtual void SetSkyboxColor(Color color) = 0;
    virtual Skybox *GetSkybox() const = 0;
    virtual Color GetClearColor() const = 0;
    virtual class IUIManager *GetUIManager() const = 0;
    virtual class EditorPanelManager *GetPanelManager() const = 0;

    // Play Mode Management
    virtual void StartPlayMode() = 0;
    virtual void StopPlayMode() = 0;
    virtual bool IsInPlayMode() const = 0;
    virtual void BuildGame() = 0;
    virtual void RunGame() = 0;

    // Debug Visualization
    virtual bool IsWireframeEnabled() const = 0;
    virtual void SetWireframeEnabled(bool enabled) = 0;
    virtual bool IsCollisionDebugEnabled() const = 0;
    virtual void SetCollisionDebugEnabled(bool enabled) = 0;

    // Editor Mode Management
    virtual EditorMode GetEditorMode() const = 0;
    virtual void SetEditorMode(EditorMode mode) = 0;
    virtual bool IsUIDesignMode() const = 0;
};

#endif // IEDITOR_H
