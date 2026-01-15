//@	{"dependencies_extra":[{"ref": "./utils.o", "rel": "implementation"}]}

#ifndef PROG_UTILS_HPP
#define PROG_UTILS_HPP

#include <string>
#include <cmath>

/**
 * \brief Contains various utility functions
 */
namespace prog::utils
{
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
			static_cast<double>(num_bytes)*8.0
				/std::log2(static_cast<double>(num_printable_ascii_chars))
			+ 0.5
		);
	}

	/**
	 * \brief The number of printable ASCII characters requires for 16 butes
	 */
	constexpr size_t num_chars_16_bytes = byte_count_to_printable_ascii_string_length(16);
};

#endif