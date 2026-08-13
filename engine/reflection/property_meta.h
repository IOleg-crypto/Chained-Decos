#ifndef CH_REFLECTION_PROPERTY_META_H
#define CH_REFLECTION_PROPERTY_META_H

#include "engine/common/base.h"
#include <string>

namespace Chained
{
	// Widget rendering hints for UI properties
	struct CH_API PropertyMeta
	{
		enum class WidgetHint
		{
			Default,	 // Auto-select based on type
			Slider,		 // Use slider instead of input
			Input,		 // Use text input
			Checkbox,	 // Force checkbox
			ColorPicker, // Color picker
			FilePicker,	 // File picker
			Enum		 // Enum dropdown
		} Hint = WidgetHint::Default;

		float MinValue = 0.0f;
		float MaxValue = 0.0f;
		float Speed = 0.1f;
		std::string Tooltip;
		bool ReadOnly = false;
		bool Transient = false;
		bool Hidden = false;

		// Convenience constructors
		PropertyMeta() = default;
		PropertyMeta(WidgetHint h)
			: Hint(h)
		{
		}
		PropertyMeta(float min, float max, float spd = 0.1f)
			: Hint(WidgetHint::Slider),
			  MinValue(min),
			  MaxValue(max),
			  Speed(spd)
		{
		}
	};
} // namespace Chained

#endif // CH_REFLECTION_PROPERTY_META_H
