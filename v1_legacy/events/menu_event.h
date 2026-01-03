#ifndef CD_EVENTS_MENU_EVENT_H
#define CD_EVENTS_MENU_EVENT_H

#include "event.h"
#include <string>

namespace CHEngine
{

enum class MenuEventType : uint8_t
{
    None = 0,
    StartGame,
    ResumeGame,
    OpenOptions,
    OpenCredits,
    OpenVideoSettings,
    OpenAudioSettings,
    OpenControlSettings,
    BackToMain,
    ExitGame,
    SelectMap,
    StartGameWithMap
};

class MenuEvent : public Event
{
public:
    MenuEvent(MenuEventType type, const std::string &mapName = "")
        : m_MenuEventType(type), m_MapName(mapName)
    {
    }

    MenuEventType GetMenuEventType() const
    {
        return m_MenuEventType;
    }
    const std::string &GetMapName() const
    {
        return m_MapName;
    }

    EVENT_CLASS_TYPE(Menu)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

    virtual std::string ToString() const override
    {
        return "MenuEvent: " + std::to_string(static_cast<int>(m_MenuEventType)) +
               (m_MapName.empty() ? "" : " (Map: " + m_MapName + ")");
    }

private:
    MenuEventType m_MenuEventType;
    std::string m_MapName;
};

} // namespace CHEngine

#endif // CD_EVENTS_MENU_EVENT_H
