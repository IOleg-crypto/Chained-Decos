#ifndef CH_INPUT_EVENTS_H
#define CH_INPUT_EVENTS_H

#include "engine/core/key_codes.h"
#include "engine/core/mouse_codes.h"
#include "events.h"

// --- Keyboard Events ---
namespace Chained
{
class KeyEvent : public Event
{
public:
    KeyCode GetKeyCode() const
    {
        return m_KeyCode;
    }
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
protected:
    KeyEvent(KeyCode keycode)
        : m_KeyCode(keycode)
    {
    }
    KeyCode m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(KeyCode keycode, bool isRepeat = false)
        : KeyEvent(keycode),
          m_IsRepeat(isRepeat)
    {
    }
    bool IsRepeat() const
    {
        return m_IsRepeat;
    }
    EVENT_CLASS_TYPE(KeyPressed)
private:
    bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(KeyCode keycode)
        : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(KeyCode keycode)
        : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyTyped)
};

// --- Mouse Events ---
class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y)
        : m_MouseX(x),
          m_MouseY(y)
    {
    }
    float GetX() const
    {
        return m_MouseX;
    }
    float GetY() const
    {
        return m_MouseY;
    }
    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_MouseX, m_MouseY;
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset),
          m_YOffset(yOffset)
    {
    }
    float GetXOffset() const
    {
        return m_XOffset;
    }
    float GetYOffset() const
    {
        return m_YOffset;
    }
    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_XOffset, m_YOffset;
};

class MouseButtonEvent : public Event
{
public:
    enum class Action
    {
        None = 0,
        Pressed,
        Released
    };
    MouseCode GetMouseButton() const
    {
        return m_Button;
    }
    Action GetAction() const
    {
        return m_Action;
    }
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
protected:
    MouseButtonEvent(MouseCode button, Action action)
        : m_Button(button),
          m_Action(action)
    {
    }
    MouseCode m_Button;
    Action m_Action;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(MouseCode button)
        : MouseButtonEvent(button, Action::Pressed)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(MouseCode button)
        : MouseButtonEvent(button, Action::Released)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonReleased)
};
} // namespace Chained
#endif // CH_INPUT_EVENTS_H