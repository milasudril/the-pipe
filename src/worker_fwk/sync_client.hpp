#ifndef PIPE_WORKER_FWK_SYNC_CLIENT_HPP
#define PIPE_WORKER_FWK_SYNC_CLIENT_HPP

#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/worker_fwk/sync_message_channel.hpp"
#include "src/worker_sync/worker_sync.hpp"

namespace Pipe::worker_fwk
{
	struct sync_message_channel_client_traits
	{
		using fd_tag = os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;
		using incoming_sync_msg_type = worker_sync::server_to_client_message;
		using outgoing_sync_msg_type = worker_sync::client_to_server_message;
		struct client_activity{};
		using sync_fd_activity_event_handler_registred_event =
			os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using sync_fd_activity_event = os_services::fd::activity_event<client_activity, fd_tag>;
	};

	class sync_client:
		public sync_message_channel<sync_message_channel_client_traits>,
		public sync_message_channel_client_traits
	{
	public:
		explicit sync_client(size_t buffer_size):sync_message_channel{buffer_size}
		{}

		template<class ... Args>
		void handle_response(Args&&...){}

		using sync_message_channel<sync_message_channel_client_traits>::handle_message;
	};

	struct client_info
	{
		os_services::fd::event_handler_id event_handler_id;
	};

	inline client_info make_sync_client(
		os_services::fd::activity_event_handler_store& event_handler_store,
		std::string const& socket_name
	)
	{
		return client_info{
			.event_handler_id = event_handler_store.add<sync_client::client_activity>(
				sync_client{65536},
				os_services::ipc::make_connection<SOCK_STREAM>(
					os_services::ipc::make_abstract_sockaddr_un(socket_name)
				),
				Pipe::os_services::fd::activity_status::read
			)
		};
	}
}

#endif
