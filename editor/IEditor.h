#ifndef IEDITOR_H
#define IEDITOR_H

#include <core/utils/Base.h>
#include <string>

#include "editor/EditorTypes.h"
#include <scene/camera/core/CameraController.h>
#include <scene/resources/map/core/SceneLoader.h>
#include <scene/resources/map/skybox/skybox.h>
#include <scene/resources/model/core/Model.h>

#include "editor/logic/IEditorState.h"
#include "editor/logic/ISceneManager.h"
#include "editor/logic/ISelectionManager.h"
#include "editor/logic/ProjectManager.h"

class IEditor
{
public:
    virtual ~IEditor() = default;

    // Sub-manager Accessors
    virtual IProjectManager &GetProjectManager() = 0;
    virtual ISceneManager &GetSceneManager() = 0;
    virtual ISelectionManager &GetSelectionManager() = 0;
    virtual IEditorState &GetState() = 0;
    virtual class IUIManager &GetUIManager() = 0;
    virtual class EditorPanelManager &GetPanelManager() = 0;

    // Service Accessors
    virtual CHEngine::Ref<IModelLoader> GetModelLoader() = 0;
    virtual CameraController &GetCameraController() = 0;

    // High-level Actions & Lifecycle
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void StartPlayMode() = 0;
    virtual void StopPlayMode() = 0;
    virtual bool IsInPlayMode() const = 0;
    virtual void BuildGame() = 0;
    virtual void RunGame() = 0;
};

#endif // IEDITOR_H
