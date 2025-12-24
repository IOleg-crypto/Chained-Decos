#include "EditorState.h"

EditorState::EditorState()
    : m_activeTool(SELECT), m_gridSize(50), m_editorMode(EditorMode::SCENE_3D),
      m_drawWireframe(false), m_drawCollisions(false)
{
}

bool EditorState::IsUIDesignMode() const
{
    // In a unified editor, UI design features are always logically "active"
    // when we are in the scene view.
    return true;
}
