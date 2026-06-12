#ifndef CH_UI_INPUT_SYSTEM_H
#define CH_UI_INPUT_SYSTEM_H

#include "ui_layout_system.h"
#include "entt/entt.hpp"

namespace Chained
{

class UIInputSystem
{
public:
    void Update(entt::registry& registry, const UILayoutSystem& layout, int inputCooldown);
};

} // namespace Chained

#endif // CH_UI_INPUT_SYSTEM_H
