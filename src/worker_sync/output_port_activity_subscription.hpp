#ifndef PIPE_WORKER_SYNC_OUTPUT_PORT_ACTIVITY_SUBSCRIPTION_HPP
#define PIPE_WORKER_SYNC_OUTPUT_PORT_ACTIVITY_SUBSCRIPTION_HPP

#include "./worker_sync_msg.hpp"

#include "src/utils/unwrap.hpp"

namespace Pipe::worker_sync
{
	template<class OutputPortActivitySubscriber, class OutputPort>
	class output_port_activity_subscription
	{
	public:
		void notify_data_ready()
		{ m_subscriber.notify_data_ready(m_id); }

		void notify_client_ready()
		{
			utils::unwrap(m_output_port).notify_client_ready(this, m_id);
		}

	private:
		worker_sync::port_activity_subscription_id m_id;
		OutputPortActivitySubscriber m_subscriber;
		OutputPort m_output_port;

	};
}
#endif