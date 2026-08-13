#ifndef CH_REFLECTION_RFL_H
#define CH_REFLECTION_RFL_H

#include <string>

#include "engine/reflection/reflection.h"

namespace Chained
{
	struct UIMeta
	{
		PropertyMeta::WidgetHint Hint = PropertyMeta::WidgetHint::Default;
		float Min = 0.0f;
		float Max = 0.0f;
		float Speed = 0.1f;
		bool ReadOnly = false;
		bool Transient = false;
		bool Hidden = false;
		const char* Tooltip = nullptr;
		const char* Extensions = nullptr;
	};

	struct AssetPath
	{
		std::string Path;
		const char* Extensions = "*.*";

		AssetPath() = default;
		AssetPath(const std::string& path, const char* ext = "*.*")
			: Path(path),
			  Extensions(ext)
		{
		}
	};

	struct ExtractedFieldMeta
	{
		PropertyMeta Meta;
		const char* Extensions = nullptr;
	};

#define CH_MARK_RFL(Type)                                                                                              \
	template <> struct is_rfl_component<Type> : std::true_type                                                         \
	{                                                                                                                  \
	};

} // namespace Chained

#endif // CH_REFLECTION_RFL_H
