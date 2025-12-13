#ifndef GUI_LAYOUT_H
#define GUI_LAYOUT_H

#include <raylib.h>
#include <stack>

namespace gui
{

// Simple layout helper for automatic positioning
class GUILayout
{
public:
    enum class Direction
    {
        Vertical,
        Horizontal
    };

    // Begin a layout context
    static void Begin(Vector2 startPos, Direction dir = Direction::Vertical, float spacing = 10.0f);

    // Get next position for an element
    static Vector2 Next(Vector2 size);

    // End current layout context
    static void End();

    // Get current cursor position
    static Vector2 GetCursor();

    // Manually advance cursor (useful for custom spacing)
    static void Advance(Vector2 offset);

private:
    struct LayoutState
    {
        Vector2 cursor;
        Direction direction;
        float spacing;
    };

    static std::stack<LayoutState> s_layoutStack;
};

} // namespace gui

#endif // GUI_LAYOUT_H
