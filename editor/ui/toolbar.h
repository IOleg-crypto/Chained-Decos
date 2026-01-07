#ifndef CH_UI_TOOLBAR_H
#define CH_UI_TOOLBAR_H

#include <functional>

namespace CH
{
namespace UI
{

void DrawToolbar(bool isPlaying, std::function<void()> onPlay, std::function<void()> onStop);

} // namespace UI
} // namespace CH

#endif // CH_UI_TOOLBAR_H
