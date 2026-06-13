#ifndef PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP
#define PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP

#include "./sync_message_channel.hpp"

#include "./port_activity_subscriber.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/utils/unwrap.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <queue>
#include <fcntl.h>

namespace Pipe::worker_fwk
{
	struct sync_message_channel_server_traits
	{
		using fd_tag = os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;
		using incoming_sync_msg_type = worker_sync::client_to_server_message;
		using outgoing_sync_msg_type = worker_sync::server_to_client_message;
		struct client_activity{};
		using sync_fd_activity_event_handler_registred_event =
			os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using sync_fd_activity_event = os_services::fd::activity_event<client_activity, fd_tag>;
	};

	template<port_activity_subscription_registry PortActivitySubscriptionRegistry>
	class sync_client_connection:
		public sync_message_channel<sync_message_channel_server_traits>,
		public sync_message_channel_server_traits
	{
	public:
		using client_activity_event_handler_registered_event = sync_fd_activity_event_handler_registred_event;
		using client_activity_event = sync_fd_activity_event;

		explicit sync_client_connection(PortActivitySubscriptionRegistry port_activity_subscriber_registry, size_t buffer_size = 65536):
			sync_message_channel<sync_message_channel_server_traits>{buffer_size},
			m_port_activity_subscriber_registry{port_activity_subscriber_registry}
		{}

		~sync_client_connection()
		{
			utils::unwrap(m_port_activity_subscriber_registry).remove_port_activity_subscriber(
				port_activity_subscriber_ref{*this}
			);
		}

		sync_client_connection(sync_client_connection&&) = default;
		sync_client_connection& operator=(sync_client_connection&&) = default;
		sync_client_connection(sync_client_connection const&) = delete;
		sync_client_connection& operator=(sync_client_connection const&) = delete;

		void handle_request(
			worker_sync::port_activity_subscription_request&& msg,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		)
		{
			auto const subscription_id = utils::unwrap(m_port_activity_subscriber_registry).add_port_activity_subscription(
				msg.server_portname,
				port_activity_subscriber_ref{*this}
			);
			ec.enable_exception_rethrow();
			send(
				worker_sync::port_activity_subscription_response{
					.id = subscription_id
				},
				tx_id
			);
		}

		void handle_request(
			worker_sync::port_activity_unsubscription msg,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		)
		{
			utils::unwrap(m_port_activity_subscriber_registry).remove_port_activity_subscription(
				msg.id,
				port_activity_subscriber_ref{*this}
			);

			ec.enable_exception_rethrow();
			send(
				worker_sync::port_activity_unsubscription_response{
					.id = msg.id
				},
				tx_id
			);
		}

		void handle_response(
			worker_sync::error_response const& err,
			worker_sync::transaction_id,
			worker_sync::exception_controller& ec
		)
		{
			ec.enable_exception_rethrow();
			throw std::runtime_error{err.content()};
		}

		void handle_message(worker_sync::client_ready_event event)
		{
			utils::unwrap(m_port_activity_subscriber_registry).notify_client_ready(
				event.id,
				port_activity_subscriber_ref{*this}
			);
		}

		void notify_data_ready(worker_sync::port_activity_subscription_id id)
		{
			send(
				worker_sync::data_ready_event{
					.id = id
				},
				worker_sync::transaction_id{}
			);
		}

		using sync_message_channel<sync_message_channel_server_traits>::handle_message;

	private:
		PortActivitySubscriptionRegistry m_port_activity_subscriber_registry;
	};


	template<port_activity_subscription_registry PortActivitySubscriptionRegistry>
	sync_client_connection(std::reference_wrapper<PortActivitySubscriptionRegistry>, size_t)->
		sync_client_connection<std::reference_wrapper<PortActivitySubscriptionRegistry>>;
}
#endif
