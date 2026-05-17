#include "./port_activity_subscription.hpp"

#include <flat_map>

namespace Pipe::worker_fwk
{
	class subscription_registry
	{
	public:
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
		class output_port_info
		{
		public:
			void inc_num_subscribers()
			{ ++m_num_subscribers; }

			void dec_num_subscribers()
			{ --m_num_subscribers; }

			void inc_num_ready_subscribers()
			{ ++m_num_ready_subscribers; }

			void dec_num_ready_subscribers()
			{ --m_num_ready_subscribers; }

		private:
			size_t m_num_subscribers{};
			size_t m_num_subscribers{};
		};

		std::unordered_map<
			worker_sync::port_activity_subscription_id, port_id> m_port_activity_subscriptions;
	};
}