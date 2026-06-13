#ifndef PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP
#define PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP

#include "src/worker_sync/worker_sync_msg.hpp"
#include "src/utils/unwrap.hpp"

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
		port_activity_subscriber_ref() = default;

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
		void* m_object = nullptr;
		void (*m_notify_data_ready)(void*, worker_sync::port_activity_subscription_id) = nullptr;
	};

	template<class T>
	concept port_activity_subscription_registry = requires(
		T obj,
		std::string const& str,
		worker_sync::port_activity_subscription_id subscription,
		port_activity_subscriber_ref subscriber
	)
	{
		{ utils::unwrap(obj).add_port_activity_subscription(str, subscriber) } -> std::same_as<worker_sync::port_activity_subscription_id>;
		{ utils::unwrap(obj).remove_port_activity_subscription(subscription, subscriber) } -> std::same_as<void>;
		{ utils::unwrap(obj).notify_client_ready(subscription, subscriber) } -> std::same_as<void>;
		{ utils::unwrap(obj).remove_port_activity_subscriber(subscriber) } -> std::same_as<void>;
	};
}
#endif