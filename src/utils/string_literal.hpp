#ifndef PIPE_UTILS_STRING_LITERAL_HPP
#define PIPE_UTILS_STRING_LITERAL_HPP

#include <algorithm>
#include <string_view>
#include <array>

namespace Pipe::utils
{
	template<size_t N>
	struct string_literal
	{
		constexpr string_literal(char const (&val)[N + 1])
		{ std::copy(std::begin(val), std::end(val) - 1, std::begin(value)); }


		constexpr operator std::string_view() const
		{ return std::string_view{value, N}; }

		constexpr auto operator<=>(string_literal const&) const = default;

		// NOTE: This is public in order to make the type structural
		char value[N];

		constexpr auto size() const
		{ return N; }

		constexpr auto begin() const
		{ return value; }

		constexpr auto end() const
		{ return value + N; }
	};

	struct string_literal_view
	{
		string_literal_view() = default;

		constexpr string_literal_view(std::string_view other):
			length{std::size(other)},
			ptr{std::data(other)}
		{}

		constexpr operator std::string_view() const
		{ return std::string_view{ptr, length}; }

		// NOTE: These are public in order to make the type structural
		size_t length;
		char const* ptr;
	};

	template<size_t N>
	string_literal(char const (&val)[N]) -> string_literal<N - 1>;

	template<auto... StringLiterals>
	consteval auto make_string_literal_array()
	{
		return std::array{
			string_literal_view{std::string_view{StringLiterals}}...
		};
	}
};

#endif