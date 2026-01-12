//@	{"target":{"name":"pipe.test"}}

#include "./pipe.hpp"
#include "src/io/io.hpp"
#include "testfwk/validation.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(prog_ipc_pipe_create_and_do_stuff)
{
	prog::ipc::pipe the_pipe;
	std::string_view msg{"Hello, World"};
	auto write_result = prog::io::write(the_pipe.write_end(), std::as_bytes(std::span{msg}));
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	std::array<char, 12> msg_read{};
	auto read_result = prog::io::read(the_pipe.read_end(), std::as_writable_bytes(std::span{msg_read}));
	EXPECT_EQ(read_result.bytes_transferred(), 12);
	EXPECT_EQ(msg, std::string_view{msg_read});

	EXPECT_NE(the_pipe.read_end(), nullptr);
	EXPECT_NE(the_pipe.write_end(), nullptr);
	the_pipe.close_read_end();
	EXPECT_EQ(the_pipe.read_end(), nullptr);
	EXPECT_NE(the_pipe.write_end(), nullptr);
	the_pipe.close_write_end();
	EXPECT_EQ(the_pipe.read_end(), nullptr);
	EXPECT_EQ(the_pipe.write_end(), nullptr);
}