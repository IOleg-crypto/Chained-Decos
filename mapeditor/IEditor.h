#ifndef IEDITOR_H
#define IEDITOR_H

#include <core/utils/Base.h>
#include <string>
#include <vector>

#include "mapeditor/EditorTypes.h"
#include <scene/camera/core/CameraController.h>
#include <scene/resources/map/core/MapLoader.h>
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

    // Map and Scene State
    virtual bool IsSceneModified() const = 0;
    virtual void SetSceneModified(bool modified) = 0;
    virtual const std::string &GetCurrentMapPath() const = 0;
    virtual void SaveMap(const std::string &path = "") = 0;
    virtual void LoadMap(const std::string &path) = 0;
    virtual GameMap &GetGameMap() = 0;

    // Tools and Grid
    virtual Tool GetActiveTool() const = 0;
    virtual void SetActiveTool(Tool tool) = 0;
    virtual int GetGridSize() const = 0;
    virtual void SetGridSize(int size) = 0;
    virtual void CreateDefaultObject(MapObjectType type, const std::string &modelName = "") = 0;

    // Service Accessors
    virtual ChainedDecos::Ref<IModelLoader> GetModelLoader() = 0;
    virtual CameraController &GetCameraController() = 0;

    // Skybox Operations
    virtual void SetSkybox(const std::string &name) = 0;
    virtual void SetSkyboxTexture(const std::string &texturePath) = 0;
    virtual void SetSkyboxColor(Color color) = 0;
    virtual Skybox *GetSkybox() const = 0;
};

#endif // IEDITOR_H


