//@	{"target":{"name":"sync_client_connection.o"}}

#include "./sync_server.hpp"

Pipe::worker_fwk::sync_client_connection::~sync_client_connection()
{
	for(auto const& item :m_port_activity_subscriptions)
	{ m_port_activity_subscriber_registry.remove_port_activity_subscription(item.second.id, port_activity_subscriber_ref{*this}, item.first); }
	// TODO: Add support for connection closed
}

void Pipe::worker_fwk::sync_client_connection::handle_request(
	worker_sync::port_activity_subscription_request&& msg,
	worker_sync::transaction_id tx_id
)
{
	utils::maybe_at_scope_exit restore_subscription_id{
		[this, subcription_id = m_port_activity_subscription_id](){
			m_port_activity_subscription_id = subcription_id;
		}
	};
	auto const subscription_id = m_port_activity_subscription_id.next();
	auto const port_id = m_port_activity_subscriber_registry.add_port_activity_subscription(
		msg.server_portname,
		port_activity_subscriber_ref{*this},
		subscription_id
	);
	utils::maybe_at_scope_exit remove_port_activity_subscription{
		[this, port_id, subscription_id](){
			m_port_activity_subscriber_registry.remove_port_activity_subscription(
				port_id,
				port_activity_subscriber_ref{*this},
				subscription_id
			);
		}
	};
	auto const i = m_port_activity_subscriptions.insert(
		std::pair{
			subscription_id,
			output_port_info{
				.id = port_id,
				.client_status = client_status::ready
			}
		}
	).first;
	utils::maybe_at_scope_exit remove_subscription{
		[this, i](){
			m_port_activity_subscriptions.erase(i);
		}
	};
	send(
		worker_sync::port_activity_subscription_response{
			.id = subscription_id
		},
		tx_id
	);

	remove_subscription.reset();
	remove_port_activity_subscription.reset();
	restore_subscription_id.reset();
}
