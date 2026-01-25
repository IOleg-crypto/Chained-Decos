#ifndef CH_UI_MENU_BAR_H
#define CH_UI_MENU_BAR_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include <functional>

namespace CHEngine
{
class Panel;

namespace UI
{

struct MenuBarState
{
    bool IsPlaying = false;
    std::vector<std::shared_ptr<Panel>> *Panels = nullptr;
};

void DrawMenuBar(const MenuBarState &state, const EventCallbackFn &callback);

} // namespace UI
} // namespace CHEngine

#endif // CH_UI_MENU_BAR_H
