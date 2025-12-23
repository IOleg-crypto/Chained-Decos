#ifndef EDITOR_H
#define EDITOR_H

#include "raylib.h"
#include <imgui.h>
#include <memory>
#include <string>

#include "core/utils/Base.h"
#include "editor/IEditor.h"
#include "editor/mapgui/IUIManager.h"
#include "events/Event.h"
#include "scene/camera/core/CameraController.h"
#include "scene/resources/map/core/SceneLoader.h"
#include "scene/resources/map/skybox/skybox.h"

// Rendering and utilities
#include "editor/logic/EditorState.h"
#include "editor/logic/ProjectManager.h"
#include "editor/logic/SceneManager.h"
#include "editor/logic/SelectionManager.h"
#include "editor/panels/EditorPanelManager.h"
#include "editor/render/EditorRenderer.h"

class IEngine;

// Main editor class for ChainedEditor
class Editor : public IEditor
{
private:
    IEngine *m_engine = nullptr;

    // Subsystem managers
    std::unique_ptr<IUIManager> m_uiManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<SelectionManager> m_selectionManager;
    std::unique_ptr<EditorState> m_editorState;
    std::unique_ptr<EditorPanelManager> m_panelManager;

    // Engine resources and services
    CHEngine::Ref<CameraController> m_cameraController;
    CHEngine::Ref<IModelLoader> m_modelLoader;

    // Rendering helper
    std::unique_ptr<EditorRenderer> m_renderer;

public:
    Editor(IEngine *engine);
    ~Editor();

    // IEditor implementation
    IProjectManager &GetProjectManager() override;
    ISceneManager &GetSceneManager() override;
    ISelectionManager &GetSelectionManager() override;
    IEditorState &GetState() override;
    IUIManager &GetUIManager() override;
    EditorPanelManager &GetPanelManager() override;

    CameraController &GetCameraController() override;
    CHEngine::Ref<IModelLoader> GetModelLoader() override;

    void Update() override;
    void Render() override;
    void RenderImGui();
    void HandleInput();
    void OnEvent(CHEngine::Event &e);

    void StartPlayMode() override;
    void StopPlayMode() override;
    bool IsInPlayMode() const override;
    void BuildGame() override;
    void RunGame() override;

    // Internal initialization and resources
    void PreloadModelsFromResources();
    void LoadSpawnTexture();
    void RenderObject(const MapObjectData &obj);
    std::string GetSkyboxAbsolutePath() const;

private:
    void InitializeSubsystems();
    bool m_isInPlayMode = false;
};

#endif // EDITOR_H
