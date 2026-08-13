#ifndef CH_REFLECTION_PROPERTY_ARCHIVE_H
#define CH_REFLECTION_PROPERTY_ARCHIVE_H

#include "engine/common/base.h"
#include "engine/common/color.h"
#include "engine/reflection/property_meta.h"
#include "engine/reflection/reflection_mode.h"
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace Chained
{
	class IPropertyArchiveBase;
	class IPropertyArchive;
	template <typename T_Archive> class Properties;

	template <typename T, typename T_Archive> void ReflectFromRfl(T& component, Chained::Properties<T_Archive>& props);

	/**
	 * @brief Base interface for property archives — data serialization and composition only.
	 */
	class CH_API IPropertyArchiveBase
	{
	public:
		virtual ~IPropertyArchiveBase() = default;
		virtual ReflectionMode GetReflectionMode() const = 0;
		virtual bool Property(const char* name, int& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, float& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, bool& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, std::string& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, glm::vec2& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, glm::vec3& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, glm::vec4& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, uint64_t& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Property(const char* name, Color& value, const PropertyMeta& meta = {}) = 0;
		virtual bool Enum(const char* name, int& value, const char** names, int count,
						  const PropertyMeta& meta = {}) = 0;
		virtual bool StringEnum(const char* name, std::string& value, const std::vector<std::string>& options,
								const PropertyMeta& meta = {}) = 0;
		virtual bool Handle(const char* name, uint64_t& value, const PropertyMeta& meta = {}) = 0;
		virtual bool File(const char* name, std::string& value, const char* extensions = nullptr,
						  const PropertyMeta& meta = {}) = 0;
		virtual void BeginSequence(const char* name, size_t& size) = 0;
		virtual void EndSequence() = 0;
		virtual bool Nested(const char* name, std::function<void(IPropertyArchiveBase&)> callback) = 0;
		virtual void BeginMap(const char* name, size_t& size) = 0;
		virtual void EndMap() = 0;
		virtual bool MapNextKey(std::string& key) = 0;
	};

	/**
	 * @brief Extended interface adding UI layout and change-tracking methods.
	 */
	class CH_API IPropertyArchive : public IPropertyArchiveBase
	{
	public:
		virtual ~IPropertyArchive() = default;
		virtual void Header(const char* label) = 0;
		virtual void Separator() = 0;
		virtual bool HasChanged() const = 0;
		virtual void SetChanged(bool changed) = 0;
	};

} // namespace Chained

#endif // CH_REFLECTION_PROPERTY_ARCHIVE_H
