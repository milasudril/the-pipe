#ifndef PIPE_WORKER_FWK_SYNC_CLIENT_HPP
#define PIPE_WORKER_FWK_SYNC_CLIENT_HPP

#include "./port_activity_subscriber.hpp"

#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/utils/unwrap.hpp"
#include "src/worker_fwk/sync_message_channel.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

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

	template<input_port_activity_subscriber InputPortActivitySubscriber>
	class sync_client:
		public sync_message_channel<sync_message_channel_client_traits>,
		public sync_message_channel_client_traits
	{
	public:
		explicit sync_client(InputPortActivitySubscriber subscriber, size_t buffer_size):
			sync_message_channel{buffer_size},
			m_subscriber{std::move(subscriber)}
		{ }

		~sync_client()
		{ utils::unwrap(m_subscriber).sync_client_lost_connection_to_server(static_cast<void const*>(this)); }

		void handle_response(
			worker_sync::error_response const& err,
			worker_sync::transaction_id,
			worker_sync::exception_controller& ec
		)
		{
			ec.enable_exception_rethrow();
			throw std::runtime_error{err.content()};
		}

		void handle_message(worker_sync::data_ready_event event, worker_sync::exception_controller& ec)
		{
			ec.enable_exception_rethrow();
			utils::unwrap(m_subscriber).notify_data_ready(event.id);
		}

		void handle_response(
			worker_sync::port_activity_subscription_response const& response,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		)
		{
			ec.enable_exception_rethrow();
			utils::unwrap(m_subscriber).subscription_completed(tx_id, response.id);
		}

		void handle_response(
			worker_sync::port_activity_unsubscription_response const& response,
			worker_sync::transaction_id tx_id,
			worker_sync::exception_controller& ec
		)
		{
			ec.enable_exception_rethrow();
			utils::unwrap(m_subscriber).unsubscription_completed(tx_id);
		}

		void notify_client_ready(worker_sync::port_activity_subscription_id id)
		{
			send(
				Pipe::worker_sync::client_ready_event{
					.id = id
				},
				Pipe::worker_sync::transaction_id{}
			);
		}

		auto subscribe_to_port(std::string const& server_portname)
		{
			auto const ret = m_current_transaction_id.next();
			utils::maybe_at_scope_exit restore_tx_id{
				[this, ret]() {
					m_current_transaction_id = ret;
				}
			};

			send(
				Pipe::worker_sync::port_activity_subscription_request{
					.server_portname = server_portname
				},
				ret
			);

			restore_tx_id.reset();
			return ret;
		}

		auto unsubscribe_from_port(worker_sync::port_activity_subscription_id id)
		{
			auto const ret = m_current_transaction_id.next();
			utils::maybe_at_scope_exit restore_tx_id{
				[this, ret]() {
					m_current_transaction_id = ret;
				}
			};

			send(
				Pipe::worker_sync::port_activity_unsubscription{
					.id = id
				},
				ret
			);

			restore_tx_id.reset();
			return ret;
		}

		using sync_message_channel<sync_message_channel_client_traits>::handle_message;

	private:
		InputPortActivitySubscriber m_subscriber;
		Pipe::worker_sync::transaction_id m_current_transaction_id{0};
	};

	struct client_info
	{
		os_services::fd::event_handler_id event_handler_id;
	};

	template<class InputPortActivitySubscriber>
	inline client_info make_sync_client(
		os_services::fd::activity_event_handler_store& event_handler_store,
		InputPortActivitySubscriber subscriber,
		std::string const& socket_name
	)
	{
		return client_info{
			.event_handler_id = event_handler_store.add<sync_client<InputPortActivitySubscriber>::client_activity>(
				sync_client{std::move(subscriber), 65536},
				os_services::ipc::make_connection<SOCK_STREAM>(
					os_services::ipc::make_abstract_sockaddr_un(socket_name)
				),
				Pipe::os_services::fd::activity_status::read
			)
		};
	}
}

#endif
