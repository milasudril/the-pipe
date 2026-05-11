#ifndef PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP
#define PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP

#include "src/os_services/error_handling/system_error.hpp"
#include "src/worker_sync/worker_sync.hpp"
#include "src/os_services/error_handling/error_handling.hpp"

#include <cstdlib>

namespace Pipe::worker_fwk
{
	class port_id
	{
	public:
		constexpr port_id() = default;

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
		size_t m_value{};
	};

	template<class T>
	concept port_activity_subscriber = requires(T& obj, worker_sync::port_activity_subscription_id id)
	{
		{ obj.notify_data_ready(id) } -> std::same_as<void>;
	};

	class port_activity_subscriber_ref
	{
	public:
		template<port_activity_subscriber T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, port_activity_subscriber_ref>)
		explicit port_activity_subscriber_ref(T& object):
			m_object{&object},
			m_notify_data_ready{
				[](void* object, worker_sync::port_activity_subscription_id id) static {
					static_cast<T*>(object)->notify_data_ready(id);
				}
			}
		{}

		void notify_data_ready(worker_sync::port_activity_subscription_id id) const
		{ m_notify_data_ready(m_object, id); }

		bool operator==(port_activity_subscriber_ref const&) const = default;
		bool operator!=(port_activity_subscriber_ref const&) const = default;

	private:
		void* m_object;
		void (*m_notify_data_ready)(void*, worker_sync::port_activity_subscription_id);
	};

	template<class T>
	concept port_activity_subscriber_registry = requires(
		T& obj,
		std::string const& str,
		port_id port,
		worker_sync::port_activity_subscription_id port_activity_subscription,
		port_activity_subscriber_ref port_activity_subscriber
	)
	{
		{ obj.add_port_activity_subscription(str, port_activity_subscriber, port_activity_subscription) } -> std::same_as<port_id>;
		{ obj.remove_port_activity_subscription(port, port_activity_subscriber, port_activity_subscription) } -> std::same_as<void>;
		{ obj.notify_client_ready(port) } -> std::same_as<void>;
	};

	class port_activity_subscriber_registry_ref
	{
	public:
		template<port_activity_subscriber_registry T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, port_activity_subscriber_registry_ref>)
		explicit port_activity_subscriber_registry_ref(T& object):
			m_object{&object},
			m_notify_client_ready{
				[](void* object, port_id id) static {
					static_cast<T*>(object)->notify_client_ready(id);
				}
			},
			m_add_port_activity_subscription{
				[](
					void* object,
					std::string const& port_name,
					port_activity_subscriber_ref port_activity_subscriber,
					worker_sync::port_activity_subscription_id port_activity_subscription
				) static {
					return static_cast<T*>(object)->add_port_activity_subscription(port_name, port_activity_subscriber, port_activity_subscription);
				}
			},
			m_remove_port_activity_subscription{
				[](
					void* object,
					port_id id,
					port_activity_subscriber_ref port_activity_subscriber,
					worker_sync::port_activity_subscription_id port_activity_subscription
				) static {
					return static_cast<T*>(object)->remove_port_activity_subscription(id, port_activity_subscriber, port_activity_subscription);
				}
			},
			m_raise_fatal_error{
				[][[noreturn]](
					void* object,
					char const* message,
					os_services::error_handling::code code
				)
				{
					static_cast<T*>(object)->raise_fatal_error(message, code);
				}
			}
		{}

		void notify_client_ready(port_id id) const
		{ m_notify_client_ready(m_object, id); }

		port_id add_port_activity_subscription(
			std::string const& port_name,
			port_activity_subscriber_ref port_activity_subscriber,
			worker_sync::port_activity_subscription_id port_activity_subscription
		) const
		{ return m_add_port_activity_subscription(m_object, port_name, port_activity_subscriber, port_activity_subscription); }

		void remove_port_activity_subscription(
			port_id id,
			port_activity_subscriber_ref port_activity_subscriber,
			worker_sync::port_activity_subscription_id port_activity_subscription
		) const
		{ m_remove_port_activity_subscription(m_object, id, port_activity_subscriber, port_activity_subscription); }

		[[noreturn]] void raise_fatal_error(char const* message, os_services::error_handling::code code)
		{
			m_raise_fatal_error(m_object, message, code);
			abort();
		}

		template<class T>
		[[noreturn]] void memory_allocation_failed(std::type_identity<T>, size_t)
		{
			throw std::bad_alloc{};
		}

	private:
		void* m_object;
		void (*m_notify_client_ready)(void*, port_id);
		port_id (*m_add_port_activity_subscription)(void*, std::string const&, port_activity_subscriber_ref, worker_sync::port_activity_subscription_id);
		void (*m_remove_port_activity_subscription)(void*, port_id, port_activity_subscriber_ref, worker_sync::port_activity_subscription_id);
		void (*m_raise_fatal_error)(void*, char const*, os_services::error_handling::code);
	};
}
#endif