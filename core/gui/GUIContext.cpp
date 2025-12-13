#include "GUIContext.h"

namespace gui
{

GUIContext::GUIContext()
{
    m_cursor = {0, 0};
}

GUIContext::~GUIContext()
{
}

void GUIContext::BeginFrame()
{
    m_hotItem.clear(); // Reset hot item at start of frame (will be re-evaluated)
    // Note: Active item usually persists across frames while mouse is held

    m_cursor = {0, 0}; // Reset cursor, or wait for explicit BeginPanel
}

void GUIContext::EndFrame()
{
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        m_activeItem.clear();
    }
}

bool GUIContext::IsMouseHovering(const Rectangle &rect)
{
    return CheckCollisionPointRec(::GetMousePosition(), rect);
}

bool GUIContext::IsMouseClicked()
{
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT); // Check raw input
}

Vector2 GUIContext::GetMousePosition() const
{
    return ::GetMousePosition();
}

void GUIContext::SetHotItem(const std::string &id)
{
    m_hotItem = id;
}

void GUIContext::SetActiveItem(const std::string &id)
{
    m_activeItem = id;
}

bool GUIContext::IsHot(const std::string &id) const
{
    return m_hotItem == id;
}

bool GUIContext::IsActive(const std::string &id) const
{
    return m_activeItem == id;
}

void GUIContext::PushLayoutStart(Vector2 position)
{
    LayoutState state;
    state.startPos = position;
    state.currentPos = position;
    m_layoutStack.push(state);
    m_cursor = position;
}

void GUIContext::PopLayout()
{
    if (!m_layoutStack.empty())
    {
        m_layoutStack.pop();
        if (!m_layoutStack.empty())
        {
            m_cursor = m_layoutStack.top().currentPos;
        }
        else
        {
            m_cursor = {0, 0};
        }
    }
}

Vector2 GUIContext::GetLayoutCursor() const
{
    return m_cursor;
}

void GUIContext::AdvanceCursor(float width, float height)
{
    m_cursor.y += height + m_style.spacing;
    // Update stack top if it exists
    if (!m_layoutStack.empty())
    {
        m_layoutStack.top().currentPos = m_cursor;
    }
}

} // namespace gui
