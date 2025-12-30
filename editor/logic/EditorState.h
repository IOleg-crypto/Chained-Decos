#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "EditorTypes.h"

class EditorState
{
public:
    EditorState();
    ~EditorState() = default;

    // Tool Management
    Tool GetActiveTool() const
    {
        return m_activeTool;
    }
    void SetActiveTool(Tool tool)
    {
        m_activeTool = tool;
    }

    // Grid Management
    int GetGridSize() const
    {
        return m_gridSize;
    }
    void SetGridSize(int size)
    {
        m_gridSize = size;
    }

    // Editor Modes
    EditorMode GetEditorMode() const
    {
        return m_editorMode;
    }
    void SetEditorMode(EditorMode mode)
    {
        m_editorMode = mode;
    }
    bool IsUIDesignMode() const;

    // Rendering Flags
    bool IsWireframeEnabled() const
    {
        return m_drawWireframe;
    }
    void SetWireframeEnabled(bool enabled)
    {
        m_drawWireframe = enabled;
    }
    bool IsCollisionDebugEnabled() const
    {
        return m_drawCollisions;
    }
    void SetCollisionDebugEnabled(bool enabled)
    {
        m_drawCollisions = enabled;
    }

private:
    Tool m_activeTool = SELECT;
    int m_gridSize = 50;
    EditorMode m_editorMode = EditorMode::SCENE_3D;
    bool m_drawWireframe = false;
    bool m_drawCollisions = false;
};

#endif // EDITOR_STATE_H
