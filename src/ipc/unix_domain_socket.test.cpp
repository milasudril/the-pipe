//@	{"target": {"name":"unix_domain_socket.test"}}

#include "./unix_domain_socket.hpp"
#include "src/ipc/socket.hpp"
#include "src/utils/utils.hpp"
#include "testfwk/validation.hpp"

#include <condition_variable>
#include <sys/socket.h>
#include <testfwk/testfwk.hpp>
#include <thread>

TESTCASE(prog_ipc_unix_domain_socket_make_abstract_sockaddr_un)
{
	auto const address = prog::utils::random_printable_ascii_string(prog::utils::num_chars_16_bytes);
	auto const res = prog::ipc::make_abstract_sockaddr_un(address);
	EXPECT_EQ(res.sun_family, AF_UNIX);
	EXPECT_EQ(res.sun_path + 1, std::string_view{address});
	EXPECT_EQ(res.sun_path[0], '\0');
}

namespace
{
	class event
	{
	public:
		void wait()
		{
			std::unique_lock lock{m_mtx};
			m_cv.wait(lock, [this](){
				return m_raised;
			});
			m_raised = false;
		}

		void raise()
		{
			std::lock_guard lock{m_mtx};
			m_raised = true;
			m_cv.notify_one();
		}

	private:
		std::mutex m_mtx;
		std::condition_variable m_cv;
		bool m_raised{false};
	};
}

TESTCASE(prog_ipc_unix_domain_socket_create_sockets_and_connect)
{
	auto const client = prog::ipc::make_socket<AF_UNIX, SOCK_SEQPACKET>();
	event server_created;
	// TODO: Need an auto-generated address
	auto const sockname = prog::utils::random_printable_ascii_string(prog::utils::num_chars_16_bytes);
	auto const address = prog::ipc::make_abstract_sockaddr_un(sockname);
	std::jthread server_thread{[address, &server_created](){
		auto const server = prog::ipc::make_socket<AF_UNIX, SOCK_SEQPACKET>();
		auto const server_socket = bind_and_listen(server.get(), address, 1024);
		server_created.raise();

		auto const connection = accept(server_socket);
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
	server_created.wait();

	auto const connected_socket = connect(client.get(), address);
	auto const write_result = prog::io::write(
		connected_socket,
		std::as_bytes(std::span{std::string_view{"Hello, World"}})
	);
	EXPECT_EQ(write_result.bytes_transferred(), 12);

	shutdown(connected_socket, prog::ipc::connection_shutdown_ops::write);
}