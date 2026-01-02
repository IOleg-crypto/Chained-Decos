#ifndef CD_EVENTS_KEY_EVENT_H
#define CD_EVENTS_KEY_EVENT_H

#include "events/event.h"


namespace CHEngine
{

class KeyEvent : public Event
{
public:
    int GetKeyCode() const
    {
        return m_KeyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
protected:
    KeyEvent(int keycode) : m_KeyCode(keycode)
    {
    }

    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount)
    {
    }

    int GetRepeatCount() const
    {
        return m_RepeatCount;
    }

    std::string ToString() const override;

    EVENT_CLASS_TYPE(KeyPressed)
private:
    int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keycode) : KeyEvent(keycode)
    {
    }

    std::string ToString() const override;

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(int keycode) : KeyEvent(keycode)
    {
    }

    std::string ToString() const override;

    EVENT_CLASS_TYPE(KeyTyped)
};
} // namespace CHEngine

#endif // CD_EVENTS_KEY_EVENT_H


