//@	{"target":{"name":"string_literal.test"}}

#include "./string_literal.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_utils_string_literal_construct)
{
	static constexpr Pipe::utils::string_literal foo{"hello, world"};
	static_assert(foo == std::string_view{"hello, world"});
	static_assert(std::string_view{"hello, world"} == foo);

	EXPECT_EQ(foo, "hello, world");
	EXPECT_EQ("hello, world", foo);
}

TESTCASE(Pipe_utils_make_string_literal_array)
{
	static constexpr auto const strings = Pipe::utils::make_string_literal_array<
		Pipe::utils::string_literal{"Foo"},
		Pipe::utils::string_literal{"Bar"},
		Pipe::utils::string_literal{"Kaka"}
	>();

	EXPECT_EQ(std::string_view{strings[0]}, "Foo");
	EXPECT_EQ(std::string_view{strings[1]}, "Bar");
	EXPECT_EQ(std::string_view{strings[2]}, "Kaka");
}