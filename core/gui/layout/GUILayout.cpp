#include "GUILayout.h"
#include <stdexcept>

namespace gui
{

std::stack<GUILayout::LayoutState> GUILayout::s_layoutStack;

void GUILayout::Begin(Vector2 startPos, Direction dir, float spacing)
{
    LayoutState state;
    state.cursor = startPos;
    state.direction = dir;
    state.spacing = spacing;
    s_layoutStack.push(state);
}

Vector2 GUILayout::Next(Vector2 size)
{
    if (s_layoutStack.empty())
    {
        throw std::runtime_error("GUILayout::Next() called without Begin()");
    }

    LayoutState &state = s_layoutStack.top();
    Vector2 position = state.cursor;

    // Advance cursor based on direction
    if (state.direction == Direction::Vertical)
    {
        state.cursor.y += size.y + state.spacing;
    }
    else // Horizontal
    {
        state.cursor.x += size.x + state.spacing;
    }

    return position;
}

void GUILayout::End()
{
    if (s_layoutStack.empty())
    {
        throw std::runtime_error("GUILayout::End() called without Begin()");
    }
    s_layoutStack.pop();
}

Vector2 GUILayout::GetCursor()
{
    if (s_layoutStack.empty())
    {
        return {0, 0};
    }
    return s_layoutStack.top().cursor;
}

void GUILayout::Advance(Vector2 offset)
{
    if (!s_layoutStack.empty())
    {
        s_layoutStack.top().cursor.x += offset.x;
        s_layoutStack.top().cursor.y += offset.y;
    }
}

} // namespace gui
