#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include "IEditorState.h"

class EditorState : public IEditorState
{
public:
    EditorState();
    ~EditorState() override = default;

    // Tool Management
    Tool GetActiveTool() const override
    {
        return m_activeTool;
    }
    void SetActiveTool(Tool tool) override
    {
        m_activeTool = tool;
    }

    // Grid Management
    int GetGridSize() const override
    {
        return m_gridSize;
    }
    void SetGridSize(int size) override
    {
        m_gridSize = size;
    }

    // Editor Modes
    EditorMode GetEditorMode() const override
    {
        return m_editorMode;
    }
    void SetEditorMode(EditorMode mode) override
    {
        m_editorMode = mode;
    }
    bool IsUIDesignMode() const override;

    // Rendering Flags
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
    Tool m_activeTool = SELECT;
    int m_gridSize = 50;
    EditorMode m_editorMode = EditorMode::SCENE_3D;
    bool m_drawWireframe = false;
    bool m_drawCollisions = false;
};

#endif // EDITOR_STATE_H
