#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include "events/Event.h"
#include "editor/core/EditorContext.h"
#include "raylib.h"

// ToolManager - Handles tool selection and input processing
class ToolManager
{
public:
    explicit ToolManager(EditorContext &context);
    ~ToolManager();

    void Update(float deltaTime);
    void HandleInput();
    void OnUpdate();
    void OnEvent(CHEngine::Event &event);

    void SelectTool(Tool tool);
    Tool GetActiveTool() const;

private:
    EditorContext &m_context;

    // Tool helpers
    void HandleShortcuts(CHEngine::Event &event);
    void UpdateGizmo();
};

#endif // TOOL_MANAGER_H
