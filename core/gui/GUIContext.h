#ifndef GUI_CONTEXT_H
#define GUI_CONTEXT_H

#include "GUIStyle.h"
#include <raylib.h>
#include <stack>
#include <string>


namespace gui
{

class GUIContext
{
public:
    GUIContext();
    ~GUIContext();

    // Lifecycle
    void BeginFrame();
    void EndFrame();

    // State Management (Immediate Mode Loop)
    // ID generation based on __LINE__ or manual string is common,
    // but for simplicity we might pass unique IDs or strings.

    // Interactions
    bool IsMouseHovering(const Rectangle &rect);
    bool IsMouseClicked();

    // Styling
    const GUIStyle &GetStyle() const
    {
        return m_style;
    }
    void SetStyle(const GUIStyle &style)
    {
        m_style = style;
    }

    // Helpers
    Vector2 GetMousePosition() const;

    // Focus management
    // Simple string-based IDs for now
    void SetHotItem(const std::string &id);
    void SetActiveItem(const std::string &id);

    bool IsHot(const std::string &id) const;
    bool IsActive(const std::string &id) const;

    // Input integration
    // We can fetch Raylib input directly, but keeping it wrapped allows for flexibility

    // Layout Stack
    void PushLayoutStart(Vector2 position);
    void PopLayout();
    Vector2 GetLayoutCursor() const;
    void AdvanceCursor(float width, float height);

private:
    GUIStyle m_style;

    // Interaction State
    std::string m_hotItem;    // Item under mouse
    std::string m_activeItem; // Item currently being interacted with (e.g. mouse down)

    // Layout State
    Vector2 m_cursor;
    // We might need a stack for nested groups/panels
    struct LayoutState
    {
        Vector2 startPos;
        Vector2 currentPos;
    };
    std::stack<LayoutState> m_layoutStack;
};

} // namespace gui

#endif // GUI_CONTEXT_H
