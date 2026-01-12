#ifndef CH_UI_PROJECT_SELECTOR_H
#define CH_UI_PROJECT_SELECTOR_H

#include <functional>
#include <raylib.h>

namespace CHEngine
{
namespace UI
{

void DrawProjectSelector(bool active, Texture2D icon, std::function<void()> onNew,
                         std::function<void()> onOpen, std::function<void()> onExit);

} // namespace UI
} // namespace CHEngine

#endif // CH_UI_PROJECT_SELECTOR_H
