//@	{"target":{"name":"socket_pair.test"}}

#include "./socket_pair.hpp"
#include "src/os_services/io/io.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(prog_ipc_socket_pair_create_and_do_stuff)
{
	prog::os_services::ipc::socket_pair<SOCK_STREAM> socket_pair;
	std::string_view msg{"Hello, World"};
	auto write_result = prog::os_services::io::write(socket_pair.socket_a(), std::as_bytes(std::span{msg}));
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	std::array<char, 12> msg_read{};
	auto read_result = prog::os_services::io::read(socket_pair.socket_b(), std::as_writable_bytes(std::span{msg_read}));
	EXPECT_EQ(read_result.bytes_transferred(), 12);
	EXPECT_EQ(msg, std::string_view{msg_read});

	EXPECT_NE(socket_pair.socket_a(), nullptr);
	EXPECT_NE(socket_pair.socket_b(), nullptr);
	socket_pair.close_socket_a();
	EXPECT_EQ(socket_pair.socket_a(), nullptr);
	EXPECT_NE(socket_pair.socket_b(), nullptr);
	socket_pair.close_socket_b();
	EXPECT_EQ(socket_pair.socket_a(), nullptr);
	EXPECT_EQ(socket_pair.socket_b(), nullptr);
}