#ifndef PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP
#define PIPE_WORKER_FWK_PORT_ACTIVITY_SUBSCRIPTION_HPP

#include "src/worker_sync/worker_sync_msg.hpp"
#include "src/utils/unwrap.hpp"
#include <concepts>

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
	concept output_port_activity_subscriber = requires(T& obj, worker_sync::port_activity_subscription_id id)
	{
		{ obj.notify_data_ready(id) } -> std::same_as<void>;
	};

	class output_port_activity_subscriber_ref
	{
	public:
		output_port_activity_subscriber_ref() = default;

		template<output_port_activity_subscriber T>
		requires(!std::is_same_v<std::remove_cvref_t<T>, output_port_activity_subscriber_ref>)
		explicit output_port_activity_subscriber_ref(T& object):
			m_object{&object},
			m_notify_data_ready{
				[](void* object, worker_sync::port_activity_subscription_id id) static {
					static_cast<T*>(object)->notify_data_ready(id);
				}
			}
		{}

		void notify_data_ready(worker_sync::port_activity_subscription_id id) const
		{ m_notify_data_ready(m_object, id); }

		bool operator==(output_port_activity_subscriber_ref const&) const = default;
		bool operator!=(output_port_activity_subscriber_ref const&) const = default;

	private:
		void* m_object = nullptr;
		void (*m_notify_data_ready)(void*, worker_sync::port_activity_subscription_id) = nullptr;
	};

	template<class T>
	concept output_port_activity_subscription_registry = requires(
		T obj,
		std::string const& str,
		worker_sync::port_activity_subscription_id subscription,
		output_port_activity_subscriber_ref subscriber
	)
	{
		{ utils::unwrap(obj).add_port_activity_subscription(str, subscriber) } -> std::same_as<worker_sync::port_activity_subscription_id>;
		{ utils::unwrap(obj).remove_port_activity_subscription(subscription, subscriber) } -> std::same_as<void>;
		{ utils::unwrap(obj).notify_client_ready(subscription, subscriber) } -> std::same_as<void>;
		{ utils::unwrap(obj).remove_output_port_activity_subscriber(subscriber) } -> std::same_as<void>;
	};

	template<class T>
	concept input_port_activity_subscriber = requires(
		T obj,
		void const* conn_ptr,
		worker_sync::port_activity_subscription_id subscription_id,
		worker_sync::transaction_id tx_id
	)
	{
		typename std::remove_cvref_t<decltype(utils::unwrap(obj))>::subscription_transaction;
		typename std::remove_cvref_t<decltype(utils::unwrap(obj))>::unsubscription_transaction;

		{ utils::unwrap(obj).sync_client_lost_connection_to_server(conn_ptr) } -> std::same_as<void>;
		{ utils::unwrap(obj).notify_data_ready(subscription_id) } -> std::same_as<void>;
		{
			utils::unwrap(obj).subscription_completed(
				std::declval<typename std::remove_cvref_t<decltype(utils::unwrap(obj))>::subscription_transaction>(),
				subscription_id
			)
		} -> std::same_as<void>;
		{
			utils::unwrap(obj).unsubscription_completed(
				std::declval<typename std::remove_cvref_t<decltype(utils::unwrap(obj))>::unsubscription_transaction>()
			)
		} -> std::same_as<void>;
	};
}
#endif