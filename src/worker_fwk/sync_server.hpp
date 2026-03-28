#ifndef PIPE_WORKER_FWK_SYNC_SERVER_HPP
#define PIPE_WORKER_FWK_SYNC_SERVER_HPP

#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/utils/utils.hpp"

namespace Pipe::worker_fwk
{
	class sync_server
	{
	public:
		sync_server():
			m_socket_name{utils::random_printable_ascii_string(os_services::ipc::sunpath_maxlength)},
			m_server_socket{
				os_services::ipc::make_server_socket<SOCK_STREAM>(
					os_services::ipc::make_abstract_sockaddr_un(m_socket_name),
					1024
				)
			}
		{}

		auto const& socket_name() const
		{ return m_socket_name; }

	private:
		std::string m_socket_name;
		os_services::ipc::server_socket<SOCK_STREAM, sockaddr_un> m_server_socket;
	};
}

#endif