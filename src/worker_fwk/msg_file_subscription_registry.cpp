//@	{"target":{"name":"msg_file_subscription_registry.o"}}

#include "./msg_file_subscription_registry.hpp"
#include "src/utils/unwrap.hpp"
#include <vector>
#include <algorithm>

Pipe::worker_sync::port_activity_subscription_id
Pipe::worker_fwk::msg_file_subscription_registry::add_port_activity_subscription(
	std::string const& port_name,
	port_activity_subscriber_ref subscriber
)
{
	auto const port = [&](){
		auto const id = m_get_port_id(m_ports, port_name);
		auto const port = m_msg_file_output_ports.find(id);
		if(port == std::end(m_msg_file_output_ports))
		{ throw std::runtime_error{"The given port is not providing a message file"}; }
		return port;
	}();

	auto const id = m_current_subscription_id.next();
	utils::maybe_at_scope_exit rollback_id{
		[this, id]() noexcept{
			m_current_subscription_id = id;
		}
	};

	m_subscriptions.insert(
		std::pair{
			id,
			std::make_unique<subscription_type>(
				id,
				&port->second,
				subscriber
			)
		}
	);

	rollback_id.reset();
	return id;
}

void Pipe::worker_fwk::msg_file_subscription_registry::remove_port_activity_subscription(
	worker_sync::port_activity_subscription_id id,
	port_activity_subscriber_ref subscriber
)
{
	auto const i = m_subscriptions.find(id);
	if(i == std::end(m_subscriptions) || i->second->get_subscriber() != subscriber)
	{ throw std::runtime_error{"Invalid subscription id"}; }
	m_subscriptions.erase(i);
}

void Pipe::worker_fwk::msg_file_subscription_registry::remove_port_activity_subscriber(
	port_activity_subscriber_ref subscriber
)
{
	utils::flatmap_erase_if(
		m_subscriptions,
		[subscriber](auto const& item){
			return item.second->get_subscriber() == subscriber;
		}
	);
}

void Pipe::worker_fwk::msg_file_subscription_registry::notify_client_ready(
	worker_sync::port_activity_subscription_id id,
	port_activity_subscriber_ref subscriber
)
{
	auto const i = m_subscriptions.find(id);
	if(i == std::end(m_subscriptions))
	{ throw std::runtime_error{"Invalid subscription id"}; }

	auto& item = *(i->second);
	if(item.get_subscriber() != subscriber)
	{ throw std::runtime_error{"Invalid subscription id"}; }

	item.notify_client_ready();
}

void Pipe::worker_fwk::msg_file_subscription_registry::notify_data_ready(port_id id)
{
	auto const port = m_msg_file_output_ports.find(id);
	assert(port != std::end(m_msg_file_output_ports));
	port->second.notify_data_ready();
}