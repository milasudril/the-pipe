#ifndef PIPE_WORKER_FWK_SYNC_SERVER_HPP
#define PIPE_WORKER_FWK_SYNC_SERVER_HPP

#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <fcntl.h>

namespace Pipe::worker_fwk
{
	class sync_client
	{
	public:
		using fd_tag = os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;

		struct client_activity{};
		using client_activity_event_handler_registered_event =
			os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using client_activity_event = os_services::fd::activity_event<client_activity, fd_tag>;

		void handle_event(client_activity_event_handler_registered_event const& event)
		{
			::fcntl(event.fd.native_handle(), F_SETFD, O_NONBLOCK);
			m_registration = event;
		}

		void handle_event(client_activity_event event)
		{
			if(has_error(event.status)) [[unlikely]]
			{
				m_registration.event_handler_store->remove(m_registration.id);
				return;
			}

			if(can_read(event.status))
			{
				// TODO: Read data and decode messages
				return;
			}

			if(can_write(event.status))
			{
				// TODO: Encode messages and write data
				return;
			}
		}

		void send()
		{
			// TODO: Add stuff to a send queue and drain it as far as possible
		}

	private:
		client_activity_event_handler_registered_event m_registration;
	};

	class sync_server
	{
	public:
		using fd_tag = os_services::ipc::server_socket_tag<SOCK_STREAM, sockaddr_un>;

		struct server_socket_activity{};
		using server_activity_event_handler_registered_event =
			os_services::fd::activity_event_handler_registered_event<server_socket_activity, fd_tag>;
		using server_activity_event = os_services::fd::activity_event<server_socket_activity, fd_tag>;

		void handle_event(server_activity_event_handler_registered_event const& event)
		{ m_registration = event; }

		void handle_event(server_activity_event event)
		{
			if(event.status == os_services::fd::activity_status::read)
			{
				std::ignore = m_registration.event_handler_store->add<sync_client::client_activity>(
					sync_client{},
					accept(m_registration.fd),
					Pipe::os_services::fd::activity_status::read
				);
			}
		}

	private:
		server_activity_event_handler_registered_event m_registration;
	};

	inline auto make_sync_server(
		os_services::fd::activity_event_handler_store& event_handler_store
	)
	{
		auto socket_name = utils::random_printable_ascii_string(os_services::ipc::sunpath_maxlength);
		return event_handler_store.add<sync_server::server_socket_activity>(
			sync_server{},
			os_services::ipc::make_server_socket<SOCK_STREAM>(
				os_services::ipc::make_abstract_sockaddr_un(socket_name),
				1024
			),
			Pipe::os_services::fd::activity_status::read
		);
	}
}

#endif