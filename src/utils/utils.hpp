//@	{"dependencies_extra":[{"ref": "./utils.o", "rel": "implementation"}]}

#ifndef PROG_UTILS_HPP
#define PROG_UTILS_HPP

#include <string>
#include <cmath>
#include <vector>

/**
 * \brief Contains various utility functions
 */
namespace prog::utils
{
	/**
	 * \brief Generates an array of random bytes
	 */
	std::vector<std::byte> random_bytes(size_t n);

	/**
	 * \brief The number of printable ASCII characters (all white-space excluded)
	 */
	constexpr size_t num_printable_ascii_chars = 94;

	/**
	 * \brief Generates a random string of printable non-white-space ASCII characters
	 */
	std::string random_printable_ascii_string(size_t n);

	/**
	 * \brief Computes the number of printable ASCII characters required to represent num_bytes
	 */
	constexpr size_t byte_count_to_printable_ascii_string_length(size_t num_bytes)
	{
		//
		// num_printable_ascii_chars^x = 256^num_bytes
		// x*log2(num_printable_ascii_chars) = num_bytes*log2(256)
		// x = num_bytes*8/log2(num_printable_ascii_chars)

		return static_cast<size_t>(
			std::ceil(
				static_cast<double>(num_bytes)*8.0
					/std::log2(static_cast<double>(num_printable_ascii_chars))
			)
		);
	}

	/**
	 * \brief The number of printable ASCII characters requires for 16 bytes
	 */
	constexpr size_t num_chars_16_bytes = byte_count_to_printable_ascii_string_length(16);

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

	template<class T>
	inline constexpr decltype(auto) unwrap(T&& obj)
	{
		if constexpr(is_refwrapper<T>)
		{ return obj.get(); }
		else
		if constexpr(is_dereferenceable<T> && !is_c_style_array<T>)
		{ return *obj; }
		else
		{ return std::forward<T>(obj); }
	}
};

#endif