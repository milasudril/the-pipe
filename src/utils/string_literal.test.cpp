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