#ifndef CD_EDITOR_CORE_EDITORCONTEXT_H
#define CD_EDITOR_CORE_EDITORCONTEXT_H

#include <memory>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "editor/editor_types.h"
#include "engine/scene/camera/camera_controller.h"
#include "engine/scene/resources/map/scene_loader.h"
#include "engine/scene/resources/map/Skybox.h"

// EditorContext - Central state management for the editor
// Holds all editor state including scene, selection, tools, and view settings
class EditorContext
{
public:
    explicit EditorContext(entt::registry &registry);
    ~EditorContext();

    // Scene accessors
    GameScene &GetCurrentScene();
    const GameScene &GetCurrentScene() const;
    const std::string &GetCurrentScenePath() const;
    void SetCurrentScenePath(const std::string &path);
    bool IsSceneModified() const;
    void SetSceneModified(bool modified);

    // Selection
    int GetSelectedObjectIndex() const;
    int GetSelectedUIElementIndex() const;
    MapObjectData *GetSelectedObject();
    UIElementData *GetSelectedUIElement();
    void SelectObject(int index);
    void SelectUIElement(int index);
    void ClearSelection();

    // Tools
    Tool GetActiveTool() const;
    void SetActiveTool(Tool tool);
    int GetGridSize() const;
    void SetGridSize(int size);

    // View state
    bool IsWireframeEnabled() const;
    void SetWireframeEnabled(bool enabled);
    bool IsCollisionDebugEnabled() const;
    void SetCollisionDebugEnabled(bool enabled);
    EditorMode GetEditorMode() const;
    void SetEditorMode(EditorMode mode);
    bool IsInPlayMode() const;
    void SetInPlayMode(bool inPlayMode);

    // Resources
    CameraController &GetCamera();
    Skybox &GetSkybox();
    entt::registry &GetECSRegistry();

private:
    // Scene data
    GameScene m_currentScene;
    std::string m_currentScenePath;
    bool m_isSceneModified;

    // Selection state
    int m_selectedObjectIndex;
    int m_selectedUIElementIndex;

    // Tool state
    Tool m_activeTool;
    int m_gridSize;

    // View state
    bool m_wireframeEnabled;
    bool m_collisionDebugEnabled;
    EditorMode m_editorMode;
    bool m_isInPlayMode;

    // Resources
    std::unique_ptr<CameraController> m_camera;
    std::unique_ptr<Skybox> m_skybox;
    entt::registry &m_ecsRegistry;
};

#endif // CD_EDITOR_CORE_EDITORCONTEXT_H
