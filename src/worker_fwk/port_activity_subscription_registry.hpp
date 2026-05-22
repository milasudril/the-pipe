#include "./port_activity_subscription.hpp"

#include <flat_map>

namespace Pipe::worker_fwk
{
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