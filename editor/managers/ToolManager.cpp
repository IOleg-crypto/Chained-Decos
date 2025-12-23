#include "core/Log.h"
#include "editor/managers/ToolManager.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include "editor/core/EditorContext.h"
#include "editor/events/EditorEvents.h"

ToolManager::ToolManager(EditorContext &context) : m_context(context)
{
}

ToolManager::~ToolManager() = default;

void ToolManager::Update(float deltaTime)
{
    OnUpdate();
}

void ToolManager::HandleInput()
{
    // Handle specific input if not event-driven
}

void ToolManager::OnUpdate()
{
    UpdateGizmo();
}

void ToolManager::OnEvent(CHEngine::Event &event)
{
    HandleShortcuts(event);
}

void ToolManager::SelectTool(Tool tool)
{
    m_context.SetActiveTool(tool);
    CD_INFO("ToolManager: Selected tool %d", static_cast<int>(tool));
}

Tool ToolManager::GetActiveTool() const
{
    return m_context.GetActiveTool();
}

void ToolManager::HandleShortcuts(CHEngine::Event &event)
{
    if (event.GetEventType() == CHEngine::EventType::KeyPressed)
    {
        CHEngine::KeyPressedEvent &e = (CHEngine::KeyPressedEvent &)event;

        // Only handle shortcuts if not editing text (TODO: Check if ImGui wants input)
        // For now, simpler check

        switch (e.GetKeyCode())
        {
        case KEY_Q:
            SelectTool(Tool::SELECT);
            event.Handled = true;
            break;
        case KEY_W:
            SelectTool(Tool::MOVE);
            event.Handled = true;
            break;
        case KEY_E:
            SelectTool(Tool::ROTATE);
            event.Handled = true;
            break;
        case KEY_R:
            SelectTool(Tool::SCALE);
            event.Handled = true;
            break;
        }
    }
}

void ToolManager::UpdateGizmo()
{
    // Gizmo rendering logic will be here or in EditorLayer/GizmoRenderer
    // For now, this is a placeholder
}

