#include "BaseMenuScreen.h"
#include "../Menu.h"

bool BaseMenuScreen::RenderActionButton(const char *label, ChainedDecos::MenuEventType eventType,
                                        const ImVec2 &size)
{
    if (m_menu)
    {
        return static_cast<Menu *>(m_menu)->RenderActionButton(label, eventType, size);
    }
    return false;
}

bool BaseMenuScreen::RenderBackButton(float width)
{
    if (m_menu)
    {
        return static_cast<Menu *>(m_menu)->RenderBackButton(width);
    }
    return false;
}

void BaseMenuScreen::RenderSectionHeader(const char *title, const char *subtitle) const
{
    if (m_menu)
    {
        static_cast<Menu *>(m_menu)->RenderSectionHeader(title, subtitle);
    }
}

class Menu *BaseMenuScreen::GetMenu() const
{
    return static_cast<Menu *>(m_menu);
}
