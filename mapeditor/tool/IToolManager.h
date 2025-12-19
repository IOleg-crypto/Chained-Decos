#ifndef ITOOLMANAGER_H
#define ITOOLMANAGER_H

#include "mapeditor/EditorTypes.h"
#include <raylib.h>
#include <string>

#include "mapeditor/IEditor.h"

class IToolManager
{
public:
    virtual ~IToolManager() = default;

    virtual void SetCamera(const Camera3D &camera) = 0;
    virtual void SetActiveTool(Tool tool) = 0;
    virtual Tool GetActiveTool() const = 0;

    virtual bool ExecutePendingAction(IEditor &editor) = 0;
    virtual void SetSelectedModel(const std::string &modelName) = 0;
    virtual const std::string &GetSelectedModel() const = 0;

    virtual void HandleToolInput(bool mousePressed, const Ray &ray, IEditor &editor) = 0;
    virtual void UpdateTool(const Ray &ray, IEditor &editor) = 0;
    virtual void RenderGizmos(IEditor &editor) = 0;
};

#endif // ITOOLMANAGER_H



