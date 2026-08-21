#ifndef CH_UI_INPUT_SYSTEM_H
#define CH_UI_INPUT_SYSTEM_H

#include "ui_layout_system.h"
#include "entt/entt.hpp"

namespace Chained
{

	// Processes mouse input against UI widgets each frame.
	// Resets all widget flags, then hit-tests active widgets by Z-order.
	// When suppress is true, flags are reset but no input is processed
	// (used on the first frame after a scene load).
	void UpdateUIInput(entt::registry& registry, const UILayoutSystem& layout, bool suppress = false);

} // namespace Chained

#endif // CH_UI_INPUT_SYSTEM_H
