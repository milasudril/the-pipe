//@	{"dependencies_extra":[{"ref":"./sync_client_connection.o", "rel":"implementation"}]}

#ifndef PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP
#define PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP

#include "./sync_message_channel.hpp"

#include "./port_activity_subscription.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/worker_sync/worker_sync.hpp"

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

	class sync_client_connection:
		public sync_message_channel<sync_message_channel_server_traits>,
		public sync_message_channel_server_traits
	{
	public:
		using client_activity_event_handler_registered_event = sync_fd_activity_event_handler_registred_event;
		using client_activity_event = sync_fd_activity_event;

		explicit sync_client_connection(port_activity_subscriber_registry_ref port_activity_subscriber_registry, size_t buffer_size = 65536):
			sync_message_channel<sync_message_channel_server_traits>{buffer_size},
			m_port_activity_subscriber_registry{port_activity_subscriber_registry}
		{}

		~sync_client_connection();

		sync_client_connection(sync_client_connection&&) = default;
		sync_client_connection& operator=(sync_client_connection&&) = default;
		sync_client_connection(sync_client_connection const&) = delete;
		sync_client_connection& operator=(sync_client_connection const&) = delete;

		void handle_request(
			worker_sync::port_activity_subscription_request&& msg,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		);

		void handle_request(
			worker_sync::port_activity_unsubscription msg,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		)
		{
			auto const i = m_port_activity_subscriptions.find(msg.id);
			if(i != std::end(m_port_activity_subscriptions))
			{
				m_port_activity_subscriber_registry.remove_port_activity_subscription(
					i->second.id, port_activity_subscriber_ref{*this}, i->first
				);
				m_port_activity_subscriptions.erase(i);
			}

			ec.enable_exception_rethrow();
			send(worker_sync::port_activity_unsubscription_response{}, tx_id);
		}

		void handle_message(worker_sync::client_ready_event event)
		{
			auto const i = m_port_activity_subscriptions.find(event.id);
			if(i == std::end(m_port_activity_subscriptions))
			{ throw std::runtime_error{"Subscription id not found"}; }

			if(i->second.client_status == client_status::ready)
			{ throw std::runtime_error{"Client is already ready"}; }

			i->second.client_status = client_status::ready;
			m_port_activity_subscriber_registry.notify_client_ready(i->second.id);
		}

		void notify_data_ready(worker_sync::port_activity_subscription_id id)
		{
			auto const i = m_port_activity_subscriptions.find(id);
			assert(i != std::end(m_port_activity_subscriptions));
			send(
				worker_sync::data_ready_event{
					.id = id
				},
				worker_sync::transaction_id{}
			);
			i->second.client_status = client_status::ready;
		}

		using sync_message_channel<sync_message_channel_server_traits>::handle_message;

	private:
		port_activity_subscriber_registry_ref m_port_activity_subscriber_registry;

		enum class client_status{ready, busy};

		struct output_port_info
		{
			port_id id;
			enum client_status client_status;
		};

		std::unordered_map<worker_sync::port_activity_subscription_id, output_port_info> m_port_activity_subscriptions;
		worker_sync::port_activity_subscription_id m_port_activity_subscription_id{0};

	};
}
#endif
