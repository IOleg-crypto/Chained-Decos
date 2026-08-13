#ifndef CH_REFLECTION_TRAITS_H
#define CH_REFLECTION_TRAITS_H

#include <type_traits>
#include <variant>

namespace Chained
{
	// Helper to detect std::variant
	template <typename T> struct is_variant : std::false_type
	{
	};
	template <typename... Args> struct is_variant<std::variant<Args...>> : std::true_type
	{
	};
	template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;

	template <typename T> struct is_rfl_component : std::false_type
	{
	};

	template <typename T> struct is_vector : std::false_type
	{
	};
	template <typename T, typename Alloc> struct is_vector<std::vector<T, Alloc>> : std::true_type
	{
		using value_type = T;
	};
	template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

} // namespace Chained

#endif // CH_REFLECTION_TRAITS_H
