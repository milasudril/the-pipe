#ifndef PIPE_OS_SERVICES_IPC_SOCKET_PAIR_HPP
#define PIPE_OS_SERVICES_IPC_SOCKET_PAIR_HPP

#include "./unix_domain_socket.hpp"
#include "src/os_services/error_handling/system_error.hpp"

#include <cstdlib>

namespace Pipe::os_services::ipc
{
	template<auto SocketType>
	class socket_pair
	{
	public:
		socket_pair()
		{
			std::array<int, 2> fds{};
			auto const res = ::socketpair(AF_UNIX, SocketType, 0, std::data(fds));
			if(res == -1)
			{ throw error_handling::system_error{"Failed to create a socket pair", errno}; }

			m_socket_a = connected_socket<SocketType, sockaddr_un>(fds[0]);
			m_socket_b = connected_socket<SocketType, sockaddr_un>(fds[1]);
		}

		auto socket_a() const noexcept
		{ return m_socket_a.get(); }

		auto socket_b() const noexcept
		{ return m_socket_b.get(); }

		auto take_socket_a() noexcept
		{ return std::move(m_socket_a); }

		auto take_socket_b() noexcept
		{ return std::move(m_socket_b); }

		void close_socket_a() noexcept
		{ m_socket_a.reset(); }

		void close_socket_b() noexcept
		{ m_socket_b.reset(); }

	private:
		connected_socket<SocketType, sockaddr_un> m_socket_a;
		connected_socket<SocketType, sockaddr_un> m_socket_b;
	};
}

#endif