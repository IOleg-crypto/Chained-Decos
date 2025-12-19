#ifndef MENU_EVENT_H
#define MENU_EVENT_H

#include "core/events/Event.h"
#include "project/chaineddecos/gamegui/Menu.h"

namespace ChainedDecos
{

class MenuEvent : public Event
{
public:
    MenuEvent(MenuAction action) : m_Action(action)
    {
    }

    MenuAction GetAction() const
    {
        return m_Action;
    }

    EVENT_CLASS_TYPE(MenuAction)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    MenuAction m_Action;
};

} // namespace ChainedDecos

#endif // MENU_EVENT_H
