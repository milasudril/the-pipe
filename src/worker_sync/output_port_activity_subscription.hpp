#ifndef PIPE_WORKER_SYNC_OUTPUT_PORT_ACTIVITY_SUBSCRIPTION_HPP
#define PIPE_WORKER_SYNC_OUTPUT_PORT_ACTIVITY_SUBSCRIPTION_HPP

#include "./worker_sync_msg.hpp"

#include <utility>

namespace Pipe::worker_sync
{
	template<class OutputPortActivitySubscriber, class OutputPort>
	class output_port_activity_subscription
	{
	public:
		explicit output_port_activity_subscription(
			worker_sync::port_activity_subscription_id id,
			OutputPort* output_port,
			OutputPortActivitySubscriber subscriber
		):
			m_id{id},
			m_output_port{output_port},
			m_subscriber{subscriber}
		{ m_output_port->add_subscription(this); }

		~output_port_activity_subscription() noexcept
		{ reset(); }

		output_port_activity_subscription(output_port_activity_subscription&& other) noexcept:
			m_id{std::exchange(other.m_id, worker_sync::port_activity_subscription_id{})},
			m_output_port{std::exchange(other.m_output_port, nullptr)},
			m_subscriber{std::exchange(other.m_subscriber, OutputPortActivitySubscriber{})}
		{ }

		output_port_activity_subscription& operator=(output_port_activity_subscription&& other) noexcept
		{
			reset();
			m_id = std::exchange(other.m_id, worker_sync::port_activity_subscription_id{});
			m_subscriber = std::exchange(other.m_subscriber, OutputPortActivitySubscriber{});
			m_output_port = std::exchange(other.m_output_port, nullptr);
			return *this;
		}

		output_port_activity_subscription(output_port_activity_subscription const&) = delete;

		output_port_activity_subscription& operator=(output_port_activity_subscription const&) = delete;

		void reset() noexcept
		{
			if(m_output_port != nullptr)
			{ m_output_port->remove_subscription(this); }
		}

		void notify_data_ready()
		{ m_subscriber.notify_data_ready(m_id); }

		void notify_client_ready()
		{ m_output_port->dec_num_busy_subscribers(this); }

	private:
		worker_sync::port_activity_subscription_id m_id;
		OutputPort* m_output_port;
		OutputPortActivitySubscriber m_subscriber;
	};
}
#endif