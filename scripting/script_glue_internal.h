#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "scriptengine.h"
#include "scriptengine_services.h"
#include <Coral/StringHelper.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#define CH_SCRIPT_FUNC extern "C"

namespace Chained
{

	// Cross-platform string conversion wrappers using Coral::StringHelper.
	// On Windows: UCChar = wchar_t (UTF-16), on Linux/macOS: UCChar = char (UTF-8).
	inline std::string ToUtf8(const Coral::UCChar* str)
	{
		if (!str)
		{
			return {};
		}
		return Coral::StringHelper::ConvertWideToUtf8(str);
	}

	inline Coral::UCString ToWide(const std::string& str)
	{
		return Coral::StringHelper::ConvertUtf8ToWide(str);
	}

	// Backward-compat aliases so existing .cpp files compile without changes.
	static inline std::string ch_u16_to_string(const Coral::UCChar* ptr)
	{
		return ToUtf8(ptr);
	}
	static inline std::u16string ch_utf8_to_u16(const std::string& str)
	{
		auto wide = ToWide(str);
		return std::u16string(wide.begin(), wide.end());
	}

	// Unified string-return abstraction for glue functions.
	// Replaces per-file thread_local buffers with a single rotating pool.
	// Each thread gets 4 slots; ReturnString cycles through them so multiple
	// return values can coexist within one interop call chain.
	class GlueStringPool
	{
	public:
		static constexpr int kSlots = 4;

		// Store a string and return a stable C pointer valid until the next
		// ReturnString call that wraps around to the same slot.
		static const Coral::UCChar* ReturnString(const std::string& str)
		{
			thread_local Coral::UCString s_Buffers[kSlots];
			thread_local int s_Index = 0;

			s_Buffers[s_Index] = ToWide(str);
			const Coral::UCChar* result = s_Buffers[s_Index].c_str();
			s_Index = (s_Index + 1) % kSlots;
			return result;
		}

		// Overload for wide strings (pass-through).
		static const Coral::UCChar* ReturnString(const Coral::UCChar* str)
		{
			return str;
		}
	};

	Scene* GetActiveScene();

	Entity GetEntity(uint64_t entityID);
} // namespace Chained

#endif // CH_SCRIPT_GLUE_INTERNAL_H