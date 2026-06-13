#ifndef PIPE_WORKER_FWK_SYNC_SERVER_HPP
#define PIPE_WORKER_FWK_SYNC_SERVER_HPP

#include "./sync_client_connection.hpp"

#include "src/utils/utils.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

namespace Pipe::worker_fwk
{
	template<class PortActivitySubscriptionRegistry>
	class sync_server
	{
	public:
		explicit sync_server(PortActivitySubscriptionRegistry port_activity_subscriber_registry):
			m_port_activity_subscriber_registry{port_activity_subscriber_registry}
		{}

		using fd_tag = os_services::ipc::server_socket_tag<SOCK_STREAM, sockaddr_un>;

		struct server_socket_activity{};
		using activity_event_handler_registered_event =
			os_services::fd::activity_event_handler_registered_event<server_socket_activity, fd_tag>;
		using server_activity_event = os_services::fd::activity_event<server_socket_activity, fd_tag>;

		void handle_event(activity_event_handler_registered_event const& event)
		{ m_registration = event; }

		void handle_event(server_activity_event event)
		{
			if(event.status == os_services::fd::activity_status::read)
			{
				std::ignore = m_registration.event_handler_store->template add<sync_client_connection::client_activity>(
					sync_client_connection{m_port_activity_subscriber_registry},
					accept(m_registration.fd),
					Pipe::os_services::fd::activity_status::read
				);
			}
		}

	private:
		PortActivitySubscriptionRegistry m_port_activity_subscriber_registry;
		activity_event_handler_registered_event m_registration;
	};

	struct server_info
	{
		os_services::fd::event_handler_id event_handler_id;
		std::string socket_name;
	};

	template<class PortActivitySubscriptionRegistry>
	inline server_info make_sync_server(
		os_services::fd::activity_event_handler_store& event_handler_store,
		PortActivitySubscriptionRegistry port_activity_subscriber_registry
	)
	{
		using sync_server_type = sync_server<PortActivitySubscriptionRegistry>;

		auto socket_name = utils::random_printable_ascii_string(os_services::ipc::abstract_sunpath_maxlength);
		return server_info{
			.event_handler_id = event_handler_store.add<sync_server_type::server_socket_activity>(
				sync_server_type{port_activity_subscriber_registry},
				os_services::ipc::make_server_socket<SOCK_STREAM>(
					os_services::ipc::make_abstract_sockaddr_un(socket_name),
					1024
				),
				Pipe::os_services::fd::activity_status::read
			),
			.socket_name = std::move(socket_name)
		};
	}
}

#endif
