#ifndef IEDITOR_STATE_H
#define IEDITOR_STATE_H

#include "editor/EditorTypes.h"

class IEditorState
{
public:
    virtual ~IEditorState() = default;

    // Tool Management
    virtual Tool GetActiveTool() const = 0;
    virtual void SetActiveTool(Tool tool) = 0;

    // Grid Management
    virtual int GetGridSize() const = 0;
    virtual void SetGridSize(int size) = 0;

    // Editor Modes
    virtual EditorMode GetEditorMode() const = 0;
    virtual void SetEditorMode(EditorMode mode) = 0;
    virtual bool IsUIDesignMode() const = 0;

    // Rendering Flags
    virtual bool IsWireframeEnabled() const = 0;
    virtual void SetWireframeEnabled(bool enabled) = 0;
    virtual bool IsCollisionDebugEnabled() const = 0;
    virtual void SetCollisionDebugEnabled(bool enabled) = 0;
};

#endif // IEDITOR_STATE_H
