#ifndef CD_ENGINE_GUI_GUI_ELEMENT_H
#define CD_ENGINE_GUI_GUI_ELEMENT_H

#include <functional>
#include <memory>
#include <raylib.h>
#include <string>

namespace CHEngine
{
class GuiElement
{
public:
    GuiElement()
        : m_position({0, 0}), m_size({0, 0}), m_visible(true), m_enabled(true), m_focused(false)
    {
    }
    virtual ~GuiElement() = default;

    virtual void Update(float deltaTime)
    {
    }
    virtual void Render() = 0;
    virtual void HandleInput()
    {
    }

    Vector2 GetPosition() const
    {
        return m_position;
    }
    void SetPosition(Vector2 pos)
    {
        m_position = pos;
    }

    Vector2 GetSize() const
    {
        return m_size;
    }
    void SetSize(Vector2 size)
    {
        m_size = size;
    }

    bool IsVisible() const
    {
        return m_visible;
    }
    void SetVisible(bool visible)
    {
        m_visible = visible;
    }

    bool IsEnabled() const
    {
        return m_enabled;
    }
    void SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool IsFocused() const
    {
        return m_focused;
    }
    void SetFocused(bool focused)
    {
        m_focused = focused;
    }

    Rectangle GetBounds() const
    {
        return {m_position.x, m_position.y, m_size.x, m_size.y};
    }

protected:
    Vector2 m_position;
    Vector2 m_size;
    bool m_visible;
    bool m_enabled;
    bool m_focused;
};
} // namespace CHEngine

#endif
