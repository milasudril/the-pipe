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
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <queue>

namespace Pipe::worker_fwk
{
	// TODO: This should be moved to a different file
	class port_id
	{
	public:
		constexpr explicit port_id(size_t value):
			m_value{value}
		{}

		constexpr port_id next()
		{
			auto const ret = *this;
			++m_value;
			return ret;
		}

		constexpr auto value() const
		{ return m_value; }

		constexpr auto operator<=>(port_id const&) const = default;

	private:
		size_t m_value;
	};

	template<class T>
	concept subscriber_registry = requires(T& obj, std::string const& str, port_id id)
	{
		{ obj.add_subscriber(str) } -> std::same_as<port_id>;
		{ obj.remove_subscriber(id) } -> std::same_as<void>;
		{ obj.notify_client_ready(id) } -> std::same_as<void>;
	};

	class subscriber_registry_ref
	{
	public:
		template<subscriber_registry T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, subscriber_registry_ref>)
		explicit subscriber_registry_ref(T& controller):
			m_controller{&controller},
			m_notify_client_ready{
				[](void* controller, port_id id) static {
					static_cast<T*>(controller)->notify_client_ready(id);
				}
			},
			m_add_subscriber{
				[](void* controller, std::string const& port_name) static {
					return static_cast<T*>(controller)->add_subscriber(port_name);
				}
			},
			m_remove_subscriber{
				[](void* controller, port_id id) static {
					return static_cast<T*>(controller)->remove_subscriber(id);
				}
			}
		{}

		void notify_client_ready(port_id id) const
		{ m_notify_client_ready(m_controller, id); }

		port_id add_subscriber(std::string const& port_name) const
		{ return m_add_subscriber(m_controller, port_name); }

		void remove_subscriber(port_id id)
		{ m_remove_subscriber(m_controller, id); }

	private:
		void* m_controller;
		void (*m_notify_client_ready)(void*, port_id);
		port_id (*m_add_subscriber)(void*, std::string const&);
		void (*m_remove_subscriber)(void*, port_id);
	};

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

		explicit sync_client_connection(subscriber_registry_ref subscriber_registry, size_t buffer_size = 65536):
			m_subscriber_registry{subscriber_registry},
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
				send_pending_messages();
				return;
			}
		}

		void read_and_dispatch_requests();

		void send_pending_messages();

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
			send_pending_messages();
		}

		void handle_request(
			worker_sync::port_activity_subscription_request&& msg,
			worker_sync::transaction_id tx_id
		)
		{
			m_currently_received_message = msg_decoder{};
			auto const id = m_subscription_id;
			//TODO: This function needs rollback support in case of exception
			auto const port_id = m_subscriber_registry.add_subscriber(msg.server_portname);
			m_subscriptions.insert(
				std::pair{
					id,
					output_port_info{
						.id = port_id,
						.client_status = client_status::ready
					}
				}
			);
			++m_subscription_id;
			send(
				worker_sync::port_activity_subscription_response{
					.subscription_id = id
				},
				tx_id
			);
		}

		void handle_request(
			worker_sync::port_activity_unsubscription msg,
			worker_sync::transaction_id tx_id
		)
		{
			m_currently_received_message = msg_decoder{};
			auto const i = m_subscriptions.find(msg.subscription_id);
			if(i != std::end(m_subscriptions))
			{
				m_subscriber_registry.remove_subscriber(i->second.id);
				m_subscriptions.erase(i);
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

			auto const i = m_subscriptions.find(event.subscription_id);
			if(i == std::end(m_subscriptions))
			{ throw std::runtime_error{"Subscription id not found"}; }

			if(i->second.client_status == client_status::ready)
			{ throw std::runtime_error{"Client is already ready"}; }

			i->second.client_status = client_status::ready;
			m_subscriber_registry.notify_client_ready(i->second.id);
		}

	private:
		void handle_message(worker_sync::msg_header header)
		{
			m_current_transaction_id = header.tx_id;

			if(header.msg_id == std::numeric_limits<decltype(header.msg_id)>::max())
			{ throw std::runtime_error{"Invalid type-id"}; }

			m_currently_received_message = utils::make_variant<msg_decoder>(header.msg_id + 1);
		}

		void enable_write_listening();

		void disable_write_listening();

		subscriber_registry_ref m_subscriber_registry;

		enum class client_status{ready, busy};

		struct output_port_info
		{
			port_id id;
			enum client_status client_status;
		};

		std::unordered_map<uint64_t, output_port_info> m_subscriptions;
		uint64_t m_subscription_id{0};

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

	class sync_server
	{
	public:
		explicit sync_server(subscriber_registry_ref subscriber_registry):
			m_subscriber_registry{subscriber_registry}
		{}

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
					sync_client_connection{m_subscriber_registry},
					accept(m_registration.fd),
					Pipe::os_services::fd::activity_status::read
				);
			}
		}

	private:
		subscriber_registry_ref m_subscriber_registry;
		server_activity_event_handler_registered_event m_registration;
	};

	struct server_info
	{
		os_services::fd::event_handler_id event_handler_id;
		std::string socket_name;
	};

	inline server_info make_sync_server(
		os_services::fd::activity_event_handler_store& event_handler_store,
		subscriber_registry_ref subscriber_registry
	)
	{
		auto socket_name = utils::random_printable_ascii_string(os_services::ipc::abstract_sunpath_maxlength);
		return server_info{
			.event_handler_id = event_handler_store.add<sync_server::server_socket_activity>(
				sync_server{subscriber_registry},
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
