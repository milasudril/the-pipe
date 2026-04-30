#ifndef PIPE_UTILS_BITENUM_HPP
#define PIPE_UTILS_BITENUM_HPP
#include <type_traits>

template<class T>
concept bitmask_enum =
	std::is_enum_v<T> &&
	requires(T e)
{
	{enable_bitmask_operators(e)};
};

template<class T>
requires(std::is_enum_v<T>)
constexpr auto to_underlying(T value)
{
	using underlying = std::underlying_type_t<T>;
	return static_cast<underlying>(value);
}

template<bitmask_enum T>
constexpr T operator~(T value)
{ return static_cast<T>(~to_underlying(value)); }

template<bitmask_enum T>
constexpr T operator|(T a, T b)
{ return static_cast<T>(to_underlying(a) | to_underlying(b)); }

template<bitmask_enum T>
constexpr T operator&(T a, T b)
{ return static_cast<T>(to_underlying(a) & to_underlying(b)); }

template<bitmask_enum T>
constexpr T operator^(T a, T b)
{ return static_cast<T>(to_underlying(a) ^ to_underlying(b)); }

template<bitmask_enum T>
constexpr T& operator|=(T& a, T b)
{ return a = a | b; }

template<bitmask_enum T>
constexpr T& operator&=(T& a, T b)
{ return a = a & b; }

template<bitmask_enum T>
constexpr T& operator^=(T& a, T b)
{ return a = a ^ b; }

template<bitmask_enum T>
constexpr auto is_set(T a, T value)
{ return static_cast<bool>(a & value); }

#endif
