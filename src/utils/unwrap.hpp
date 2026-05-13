#ifndef PIPE_UTILS_UNWRAP_HPP
#define PIPE_UTILS_UNWRAP_HPP

#include <type_traits>
#include <cstddef>
#include <utility>

namespace Pipe::utils
{
	template<class T>
	concept reference = std::is_reference_v<T>;

	template<class T>
	concept is_refwrapper = requires(T obj)
	{
		{obj.get()} -> reference;
	};

	template<class T>
	concept is_dereferenceable = requires(T obj)
	{
		{*obj};
	};

	template<class T, size_t N>
	consteval void detect_static_array(T (&)[N]){}

	template<class T>
	concept is_c_style_array = std::is_array_v<T> || requires(T obj, size_t x)
	{
		{detect_static_array(obj)};
	};

	/**
	 * \brief A utility function to access the object behind ref
	 */
	template<class T>
	inline constexpr decltype(auto) unwrap(T&& ref)
	{
		if constexpr(is_refwrapper<T>)
		{ return ref.get(); }
		else
		if constexpr(is_dereferenceable<T> && !is_c_style_array<T>)
		{ return *ref; }
		else
		{ return std::forward<T>(ref); }
	}

	template<class T>
	concept is_comparable_to_nullptr = requires(T obj){
		{ obj == nullptr } -> std::same_as<bool>;
		{ obj != nullptr } -> std::same_as<bool>;
	};

	template<class T>
	concept stores_optional_value = requires(T obj){
		{ obj.has_value() } -> std::same_as<bool>;
	};

	template<class T>
	inline constexpr bool has_value(T const& item)
	{
		if constexpr(is_comparable_to_nullptr<T>)
		{ return item != nullptr; }
		else
		if constexpr(stores_optional_value<T>)
		{ return item.has_value(); }
		else
		{ return true; }
	}
}
#endif