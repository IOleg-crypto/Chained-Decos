#ifndef CH_REFLECTION_PROPERTIES_H
#define CH_REFLECTION_PROPERTIES_H

#include "engine/common/uuid.h"
#include "engine/reflection/property_archive.h"
#include "engine/reflection/reflection_traits.h"
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace Chained
{
	// Alias moved to end of file to resolve circular dependencies

	// The "Properties" class is the primary interface for reflection.
	// It is used by both Serializers (YAML) and Editor UI (ImGui).
	template <typename T_Archive> class Properties
	{
	public:
		Properties(T_Archive& archive)
			: m_Archive(archive)
		{
		}

		ReflectionMode GetMode() const
		{
			return m_Archive.GetReflectionMode();
		}

		bool Color(const char* name, Chained::Color& value)
		{
			return m_Archive.Property(name, value);
		}

		bool Handle(const char* name, uint64_t& value)
		{
			return m_Archive.Handle(name, value);
		}

		bool Handle(const char* name, Chained::UUID& value)
		{
			return m_Archive.Handle(name, (uint64_t&)value);
		}

		template <typename T> bool Sequence(const char* name, std::vector<T>& values)
		{
			size_t size = values.size();
			m_Archive.BeginSequence(name, size);

			if (GetMode() == ReflectionMode::Deserialize)
			{
				values.resize(size);
			}

			bool changed = false;
			for (size_t i = 0; i < values.size(); ++i)
			{
				std::string label = "[" + std::to_string(i) + "]";
				if (m_Archive.GetReflectionMode() == ReflectionMode::UI)
				{
					if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T> ||
								  std::is_enum_v<T> || (std::is_integral_v<T> && std::is_unsigned_v<T>))
					{
						if (Property(label.c_str(), values[i]))
						{
							changed = true;
						}
					}
					else if constexpr (is_rfl_component<T>::value)
					{
						if (m_Archive.Nested(label.c_str(), [&](IPropertyArchiveBase& archive) {
								Properties<IPropertyArchiveBase> props(archive);
								ReflectFromRfl(values[i], props);
							}))
						{
							changed = true;
						}
					}
					else
					{
						if (Nested(label.c_str(), values[i]))
						{
							changed = true;
						}
					}
				}
				else
				{
					// For serialization/deserialization, we don't want keys for sequence items
					if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T> ||
								  std::is_enum_v<T> || (std::is_integral_v<T> && std::is_unsigned_v<T>))
					{
						if (Property(nullptr, values[i]))
						{
							changed = true;
						}
					}
					else if constexpr (is_rfl_component<T>::value)
					{
						if (m_Archive.Nested(nullptr, [&](IPropertyArchiveBase& archive) {
								Properties<IPropertyArchiveBase> props(archive);
								ReflectFromRfl(values[i], props);
							}))
						{
							changed = true;
						}
					}
					else
					{
						if (m_Archive.Nested(nullptr, [&](IPropertyArchiveBase& archive) {
								Properties<IPropertyArchiveBase> props(archive);
								values[i].Reflect(props);
							}))
						{
							changed = true;
						}
					}
				}
			}

			m_Archive.EndSequence();
			return changed;
		}

		template <typename T> bool Nested(const char* name, T& value)
		{
			return m_Archive.Nested(name, [&](IPropertyArchiveBase& archive) {
				Properties<IPropertyArchiveBase> props(archive);
				value.Reflect(props);
			});
		}

		template <typename V> bool Map(const char* name, std::unordered_map<std::string, V>& map)
		{
			size_t size = map.size();
			m_Archive.BeginMap(name, size);

			if (GetMode() == ReflectionMode::Serialize)
			{
				for (auto& [key, value] : map)
				{
					std::string k = key;
					m_Archive.MapNextKey(k);
					Property(nullptr, value);
				}
			}
			else if (GetMode() == ReflectionMode::Deserialize)
			{
				map.clear();
				std::string key;
				for (size_t i = 0; i < size; i++)
				{
					m_Archive.MapNextKey(key);
					V value{};
					Property(nullptr, value);
					map[key] = value;
				}
			}
			// UI mode: BeginMap/EndMap are stubs; actual rendering
			// is done by UIProperties::Map() template when T_Archive = UIProperties

			m_Archive.EndMap();
			return false;
		}

		template <typename T> bool Property(const char* name, T& value)
		{
			return Property(name, value, {});
		}

		template <typename T_Enum> bool Enum(const char* name, T_Enum& value, const char** names, int count)
		{
			return Enum(name, value, names, count, {});
		}

		template <typename T_Enum> bool Property(const char* name, T_Enum& value, const char** names, int count)
		{
			return Enum(name, value, names, count);
		}

		bool File(const char* name, std::string& value, const char* extensions = nullptr)
		{
			return File(name, value, extensions, {});
		}

		bool StringEnum(const char* name, std::string& value, const std::vector<std::string>& options,
						const PropertyMeta& meta = {})
		{
			return m_Archive.StringEnum(name, value, options, meta);
		}

		// --- Property methods with metadata ---
		template <typename T> bool Property(const char* name, T& value, const PropertyMeta& meta)
		{
			if constexpr (is_variant_v<T>)
			{
				return std::visit(
					[&](auto&& v) {
						if constexpr (requires { m_Archive.Property(name, v, meta); })
						{
							return m_Archive.Property(name, v, meta);
						}
						else
						{
							return false;
						}
					},
					value);
			}
			else if constexpr (std::is_enum_v<T>)
			{
				int temp = (int)value;
				bool changed = m_Archive.Property(name, temp, meta);
				if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
				{
					value = (T)temp;
				}
				return changed;
			}
			else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, uint64_t> &&
							   !std::is_same_v<T, bool>)
			{
				uint64_t temp = static_cast<uint64_t>(value);
				bool changed = m_Archive.Property(name, temp, meta);
				if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
				{
					value = static_cast<T>(temp);
				}
				return changed;
			}
			else
			{
				return m_Archive.Property(name, value, meta);
			}
		}

		template <typename T_Enum>
		bool Enum(const char* name, T_Enum& value, const char** names, int count, const PropertyMeta& meta)
		{
			int temp = (int)value;
			bool changed = m_Archive.Enum(name, temp, names, count, meta);
			if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
			{
				value = (T_Enum)temp;
			}
			return changed;
		}

		bool File(const char* name, std::string& value, const char* extensions, const PropertyMeta& meta)
		{
			return m_Archive.File(name, value, extensions, meta);
		}

		// Post-change hook
		Properties& OnChange(std::function<void()> func)
		{
			if (m_Archive.HasChanged())
			{
				func();
			}
			return *this;
		}

		void SetChanged(bool changed)
		{
			m_Archive.SetChanged(changed);
		}
		bool HasChanged() const
		{
			return m_Archive.HasChanged();
		}
		T_Archive& GetArchive()
		{
			return m_Archive;
		}

	private:
		T_Archive& m_Archive;
	};

	/**
	 * @brief Specialized properties for when the archive type is erased.
	 */
	using GenericProperties = Properties<IPropertyArchiveBase>;

} // namespace Chained

#endif // CH_REFLECTION_PROPERTIES_H
