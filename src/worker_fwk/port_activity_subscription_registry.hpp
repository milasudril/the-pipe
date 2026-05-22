#include "./port_activity_subscription.hpp"
#include "src/utils/bound_member_function.hpp"

#include <flat_map>

namespace Pipe::worker_fwk
{
	class msg_file_subcriber_barrier
	{
	public:
		using submit_function = utils::bound_member_function<void>;

		explicit msg_file_subcriber_barrier(submit_function submit_result):
			m_submit_result{submit_result}
		{}

		void inc_num_subscribers()
		{ ++m_num_subscribers; }

		void dec_num_subscribers()
		{ --m_num_subscribers; }

		void inc_num_ready_subscribers()
		{
			++m_num_ready_subscribers;
			if(m_num_ready_subscribers == m_num_subscribers)
			{
				m_submit_result();
				m_num_ready_subscribers = 0;
			}
		}

	private:
		size_t m_num_subscribers{};
		size_t m_num_ready_subscribers{};
		submit_function m_submit_result;
	};

	class subscription_registry
	{
	public:
		template<class OutputPortCollection>
		subscription_registry(OutputPortCollection& output_ports):
			m_outputs{
				&output_ports,
				output_ports.get_msg_file_output_ports()
			}
		{}


		void notify_client_ready(worker_sync::port_activity_subscription_id id) const;

		worker_sync::port_activity_subscription_id add_port_activity_subscription(
			std::string const& port_name,
			port_activity_subscriber_ref port_activity_subscriber
		) const;

		void remove_port_activity_subscription(
			port_activity_subscriber_ref port_activity_subscriber,
			worker_sync::port_activity_subscription_id port_activity_subscription
		) const;

	private:
		std::flat_map<
			worker_sync::port_activity_subscription_id, port_id> m_port_activity_subscriptions;
	};
}