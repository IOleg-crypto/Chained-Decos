#ifndef CH_UI_PROJECT_SELECTOR_H
#define CH_UI_PROJECT_SELECTOR_H

#include <functional>
#include <raylib.h>

namespace CH
{
namespace UI
{

void DrawProjectSelector(bool active, Texture2D icon, std::function<void()> onNew,
                         std::function<void()> onOpen, std::function<void()> onExit);

} // namespace UI
} // namespace CH

#endif // CH_UI_PROJECT_SELECTOR_H
