//@	{"dependencies_extra":[{"ref":"./sync_server.o", "rel":"implementation"}]}

#ifndef PIPE_WORKER_FWK_SYNC_SERVER_HPP
#define PIPE_WORKER_FWK_SYNC_SERVER_HPP

#include "src/worker_sync/worker_sync.hpp"
#include "src/utils/utils.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <cstring>
#include <fcntl.h>
#include <type_traits>
#include <unordered_map>

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

		explicit sync_client_connection(size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<std::byte[]>(buffer_size)}
		{}

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
				read_and_dispatch_requests();
				return;
			}

			if(can_write(event.status))
			{
				send_pending_responses();
				return;
			}
		}

		void read_and_dispatch_requests();

		void send_pending_responses();

		template<class T>
		void send(T&&)
		{
			// TODO: Add stuff to a send queue and drain it as far as possible
		}

		void handle_request(worker_sync::port_activity_subscription_request&& msg)
		{
			m_currently_received_message = msg_decoder{};
			auto const id = m_subscription_id;
			m_subscriptions.insert(std::pair{id, std::move(msg.server_portname)});
			++m_subscription_id;
			send(
				worker_sync::port_activity_subscription_response{
					.transaction_id = msg.transaction_id,
					.subscription_id = id
				}
			);
		}

		void handle_request(worker_sync::port_activity_unsubscription msg)
		{
			m_currently_received_message = msg_decoder{};
			m_subscriptions.erase(msg.subscription_id);
			send(
				worker_sync::port_activity_unsubscription_response{
					.transaction_id = msg.transaction_id
				}
			);
		}

		void handle_request(worker_sync::client_ready_event)
		{
#if __GNUC__ == 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
			m_currently_received_message = msg_decoder{};
#if __GNUC__ == 13
#pragma GCC diagnostic pop
#endif
			// TODO: dispatch ready event on corresponding port
		}

	private:
		void handle_request(worker_sync::msg_header header)
		{
			if(header.msg_id == std::numeric_limits<decltype(header.msg_id)>::max())
			{ throw std::runtime_error{"Invalid type-id"}; }

			m_currently_received_message = utils::make_variant<msg_decoder>(header.msg_id + 1);
		}

		std::unordered_map<uint64_t, std::string> m_subscriptions;
		uint64_t m_subscription_id{0};

		size_t m_buffer_size;

		// Decoder
		std::unique_ptr<std::byte[]> m_input_buffer;
		std::span<std::byte const> m_bytes_left_to_process;
		msg_decoder m_currently_received_message;

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
				std::ignore = m_registration.event_handler_store->add<sync_client_connection::client_activity>(
					sync_client_connection{},
					accept(m_registration.fd),
					Pipe::os_services::fd::activity_status::read
				);
			}
		}

	private:
		server_activity_event_handler_registered_event m_registration;
	};

	struct server_info
	{
		os_services::fd::event_handler_id event_handler_id;
		std::string socket_name;
	};

	inline server_info make_sync_server(
		os_services::fd::activity_event_handler_store& event_handler_store
	)
	{
		auto socket_name = utils::random_printable_ascii_string(os_services::ipc::abstract_sunpath_maxlength);
		return server_info{
			.event_handler_id = event_handler_store.add<sync_server::server_socket_activity>(
				sync_server{},
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