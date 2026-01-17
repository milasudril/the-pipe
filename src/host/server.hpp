#include "./client_process.hpp"

#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_monitor.hpp"
#include "src/handshaking_protocol/handshaking_protocol.hpp"
#include "src/utils/utils.hpp"

#include <unordered_map>

namespace prog::host
{
	class server_activity_handler
	{
	public:
		explicit server_activity_handler(std::string_view server_name):
			m_server_name{server_name}
		{}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::ipc::server_socket_ref<SOCK_STREAM, sockaddr_un> fd
		)
		{
			if(can_read(event.get_activity_status()))
			{
				m_clients.emplace(m_client_id, accept(fd));
			}
		}

	private:
		std::string m_server_name;
		std::unordered_map<uint64_t, os_services::ipc::connected_socket<SOCK_STREAM, sockaddr_un>> m_clients;
		uint64_t m_client_id{0};
	};

	void make_server(os_services::fd::activity_monitor& activity_monitor)
	{
		auto server_name = utils::random_printable_ascii_string(
			std::tuple_size_v<handshaking_protocol::server_socket_name>
		);

		activity_monitor.add(
			os_services::ipc::make_server_socket<SOCK_STREAM>(
				os_services::ipc::make_abstract_sockaddr_un(server_name),
					1024
			),
			os_services::fd::activity_status::read,
			server_activity_handler{server_name}
		);
	}
}