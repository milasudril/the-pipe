//@	{"dependencies_extra":[{"ref":"./sync_client_connection.o", "rel":"implementation"}]}

#ifndef PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP
#define PIPE_WORKER_FWK_SYNC_CLIENT_CONNECTION_HPP

#include "./port_activity_subscription.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/worker_sync/worker_sync.hpp"

#include <queue>
#include <fcntl.h>

namespace Pipe::worker_fwk
{
	class sync_client_connection
	{
	public:
		using fd_tag = os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>;

		struct client_activity{};
		using client_activity_event_handler_registered_event =
			os_services::fd::activity_event_handler_registered_event<client_activity, fd_tag>;
		using client_activity_event = os_services::fd::activity_event<client_activity, fd_tag>;

		using msg_decoder = utils::wrap_variant_element_t<
			utils::variant_push_front_t<worker_sync::client_to_server_message, worker_sync::msg_header>,
			worker_sync::decoder
		>;

		using msg_encoder = utils::wrap_variant_element_t<
			utils::variant_push_front_t<worker_sync::server_to_client_message, worker_sync::msg_header>,
			worker_sync::encoder
		>;

		explicit sync_client_connection(port_activity_subscriber_registry_ref port_activity_subscriber_registry, size_t buffer_size = 65536):
			m_port_activity_subscriber_registry{port_activity_subscriber_registry},
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<std::byte[]>(buffer_size)},
			m_output_buffer{std::make_unique<std::byte[]>(buffer_size)}
		{}

		~sync_client_connection();

		sync_client_connection(sync_client_connection&&) = default;
		sync_client_connection& operator=(sync_client_connection&&) = default;
		sync_client_connection(sync_client_connection const&) = delete;
		sync_client_connection& operator=(sync_client_connection const&) = delete;

		void handle_event(client_activity_event_handler_registered_event const& event)
		{
			auto const fd = event.fd.native_handle();
			auto const flags = ::fcntl(fd, F_GETFL);
			assert(flags != -1);
			::fcntl(event.fd.native_handle(), F_SETFL, O_NONBLOCK|flags);
			m_registration = event;
		}

		void handle_event(client_activity_event event)
		{
			assert(m_registration.event_handler_store != nullptr);

			if(has_error(event.status)) [[unlikely]]
			{
				m_registration.event_handler_store->remove(m_registration.id);
				return;
			}

			auto connection_status = connection_status::ok;
			if(can_read(event.status))
			{ connection_status = read_and_dispatch_requests(); }

			if(can_write(event.status) && connection_status == connection_status::ok)
			{ std::ignore = send_pending_messages(); }
		}

		enum class connection_status{ok, closed};

		[[nodiscard]] connection_status read_and_dispatch_requests();

		[[nodiscard]] connection_status send_pending_messages();

		template<class T>
		void send(T&& msg, worker_sync::transaction_id tx_id)
		{
			auto const msg_id = utils::variant_index_v<T, worker_sync::server_to_client_message>;
			m_msgs_to_send.push(
				worker_sync::encoder<worker_sync::msg_header>{
					worker_sync::msg_header{
						.msg_id = msg_id,
						.tx_id = tx_id
					}
				}
			);
			m_msgs_to_send.push(
				worker_sync::encoder<std::remove_cvref_t<T>>{std::forward<T>(msg)}
			);
			std::ignore = send_pending_messages();
		}

		void handle_request(
			worker_sync::port_activity_subscription_request&& msg,
			worker_sync::transaction_id tx_id
		);

		void handle_request(
			worker_sync::port_activity_unsubscription msg,
			worker_sync::transaction_id tx_id
		)
		{
			m_currently_received_message = msg_decoder{};
			auto const i = m_port_activity_subscriptions.find(msg.id);
			if(i != std::end(m_port_activity_subscriptions))
			{
				m_port_activity_subscriber_registry.remove_port_activity_subscription(i->second.id, port_activity_subscriber_ref{*this}, i->first);
				m_port_activity_subscriptions.erase(i);
			}

			send(worker_sync::port_activity_unsubscription_response{}, tx_id);
		}

		void handle_message(worker_sync::client_ready_event event)
		{
#if __GNUC__ == 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
			m_currently_received_message = msg_decoder{};
#if __GNUC__ == 13
#pragma GCC diagnostic pop
#endif

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

	private:
		void handle_message(worker_sync::msg_header header)
		{
			utils::maybe_at_scope_exit on_exit{
				[this](){
					m_currently_received_message = msg_decoder{};
				}
			};

			m_current_transaction_id = header.tx_id;
			if(header.msg_id == std::numeric_limits<decltype(header.msg_id)>::max())
			{ throw std::runtime_error{"Invalid type-id"}; }

			m_currently_received_message = utils::make_variant<msg_decoder>(header.msg_id + 1);
			on_exit.reset();
		}

		void enable_write_listening();

		void disable_write_listening();

		port_activity_subscriber_registry_ref m_port_activity_subscriber_registry;

		enum class client_status{ready, busy};

		struct output_port_info
		{
			port_id id;
			enum client_status client_status;
		};

		std::unordered_map<worker_sync::port_activity_subscription_id, output_port_info> m_port_activity_subscriptions;
		worker_sync::port_activity_subscription_id m_port_activity_subscription_id{0};

		size_t m_buffer_size;

		// Decoder
		std::unique_ptr<std::byte[]> m_input_buffer;
		std::span<std::byte const> m_bytes_left_to_process;
		msg_decoder m_currently_received_message;
		worker_sync::transaction_id m_current_transaction_id;

		// Encoder
		std::unique_ptr<std::byte[]> m_output_buffer;
		std::span<std::byte const> m_bytes_to_write;
		std::queue<msg_encoder> m_msgs_to_send;
		bool m_is_listening_for_write{false};

		client_activity_event_handler_registered_event m_registration;
	};
}
#endif