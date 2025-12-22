#include "editor/core/EditorContext.h"
#include <fstream>

EditorContext::EditorContext(entt::registry &registry)
    : m_ecsRegistry(registry), m_isSceneModified(false), m_selectedObjectIndex(-1),
      m_selectedUIElementIndex(-1), m_activeTool(Tool::SELECT), m_gridSize(1),
      m_wireframeEnabled(false), m_collisionDebugEnabled(false), m_editorMode(EditorMode::SCENE_3D),
      m_isInPlayMode(false)
{
    m_camera = std::make_unique<CameraController>();
    m_skybox = std::make_unique<Skybox>();
}

EditorContext::~EditorContext() = default;

// Scene accessors implementation
GameScene &EditorContext::GetCurrentScene()
{
    return m_currentScene;
}

const GameScene &EditorContext::GetCurrentScene() const
{
    return m_currentScene;
}

const std::string &EditorContext::GetCurrentScenePath() const
{
    return m_currentScenePath;
}

void EditorContext::SetCurrentScenePath(const std::string &path)
{
    m_currentScenePath = path;
}

bool EditorContext::IsSceneModified() const
{
    return m_isSceneModified;
}

void EditorContext::SetSceneModified(bool modified)
{
    m_isSceneModified = modified;
}

// Selection implementation
int EditorContext::GetSelectedObjectIndex() const
{
    return m_selectedObjectIndex;
}

int EditorContext::GetSelectedUIElementIndex() const
{
    return m_selectedUIElementIndex;
}

MapObjectData *EditorContext::GetSelectedObject()
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < m_currentScene.GetMapObjects().size())
    {
        return &m_currentScene.GetMapObjectsMutable()[m_selectedObjectIndex];
    }
    return nullptr;
}

UIElementData *EditorContext::GetSelectedUIElement()
{
    if (m_selectedUIElementIndex >= 0 &&
        m_selectedUIElementIndex < m_currentScene.GetUIElements().size())
    {
        return &m_currentScene.GetUIElementsMutable()[m_selectedUIElementIndex];
    }
    return nullptr;
}

void EditorContext::SelectObject(int index)
{
    m_selectedObjectIndex = index;
    // Deselect UI element when selecting 3D object
    m_selectedUIElementIndex = -1;
}

void EditorContext::SelectUIElement(int index)
{
    m_selectedUIElementIndex = index;
    // Deselect 3D object when selecting UI element
    m_selectedObjectIndex = -1;
}

void EditorContext::ClearSelection()
{
    m_selectedObjectIndex = -1;
    m_selectedUIElementIndex = -1;
}

// Tools implementation
Tool EditorContext::GetActiveTool() const
{
    return m_activeTool;
}

void EditorContext::SetActiveTool(Tool tool)
{
    m_activeTool = tool;
}

int EditorContext::GetGridSize() const
{
    return m_gridSize;
}

void EditorContext::SetGridSize(int size)
{
    m_gridSize = size;
}

// View state implementation
bool EditorContext::IsWireframeEnabled() const
{
    return m_wireframeEnabled;
}

void EditorContext::SetWireframeEnabled(bool enabled)
{
    m_wireframeEnabled = enabled;
}

bool EditorContext::IsCollisionDebugEnabled() const
{
    return m_collisionDebugEnabled;
}

void EditorContext::SetCollisionDebugEnabled(bool enabled)
{
    m_collisionDebugEnabled = enabled;
}

EditorMode EditorContext::GetEditorMode() const
{
    return m_editorMode;
}

void EditorContext::SetEditorMode(EditorMode mode)
{
    m_editorMode = mode;
}

bool EditorContext::IsInPlayMode() const
{
    return m_isInPlayMode;
}

void EditorContext::SetInPlayMode(bool inPlayMode)
{
    m_isInPlayMode = inPlayMode;
}

// Resources implementation
CameraController &EditorContext::GetCamera()
{
    return *m_camera;
}

Skybox &EditorContext::GetSkybox()
{
    return *m_skybox;
}

entt::registry &EditorContext::GetECSRegistry()
{
    return m_ecsRegistry;
}
