//@	{"target": {"name":"unix_domain_socket.test"}}

#include "./unix_domain_socket.hpp"
#include "src/ipc/socket.hpp"
#include "testfwk/validation.hpp"

#include <sys/socket.h>
#include <testfwk/testfwk.hpp>
#include <thread>

TESTCASE(prog_ipc_unix_domain_socket_make_abstract_sockaddr_un)
{
	auto const res = prog::ipc::make_abstract_sockaddr_un("testsocket");
	EXPECT_EQ(res.sun_family, AF_UNIX);
	EXPECT_EQ(res.sun_path + 1, std::string_view{"testsocket"});
	EXPECT_EQ(res.sun_path[0], '\0');
}

TESTCASE(prog_ipc_unix_domain_socket_create_sockets_and_connect)
{
	auto const client = prog::ipc::make_socket<AF_UNIX, SOCK_SEQPACKET>();

	// TODO: Need an auto-generated address
	auto const address = prog::ipc::make_abstract_sockaddr_un("testsocket");
	std::jthread server_thread{[address](){
		auto const server = prog::ipc::make_socket<AF_UNIX, SOCK_SEQPACKET>();
		auto const listening_socket = bind(server.get(), address);
		listen(listening_socket, 1024);
		auto const connection = accept(listening_socket);
		std::array<char, 32> buffer{};
		auto const read_result = prog::io::read(
			connection.get(),
			std::as_writable_bytes(std::span{buffer})
		);
		EXPECT_EQ(read_result.bytes_transferred(), 12);
		EXPECT_EQ((std::string_view{std::data(buffer), 12}), "Hello, World");
		auto const no_read_results = prog::io::read(
			connection.get(),
			std::as_writable_bytes(std::span{buffer})
		);
		EXPECT_EQ(no_read_results.bytes_transferred(), 0);
	}};

	std::this_thread::sleep_for(std::chrono::seconds{2});
	auto const connected_socket = connect(client.get(), address);
	auto const write_result = prog::io::write(
		connected_socket,
		std::as_bytes(std::span{std::string_view{"Hello, World"}})
	);
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	shutdown(connected_socket, prog::ipc::connection_shutdown_ops::write);
}