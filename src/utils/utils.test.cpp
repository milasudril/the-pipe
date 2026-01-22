//@	{"target":{"name":"utils.test"}}

#include "./utils.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_utils_random_printable_ascii_string)
{
	auto const string = Pipe::utils::random_printable_ascii_string(16);
	EXPECT_EQ(std::size(string), 16);
	printf("%s\n", string.c_str());
}

TESTCASE(Pipe_utils_byte_count_to_printable_ascii_string_length)
{
	EXPECT_EQ(Pipe::utils::byte_count_to_printable_ascii_string_length(16), 20);
	static_assert(Pipe::utils::num_chars_16_bytes == 20);
}