//	{"target":{"name":"msg_file_subscription_registry.o"}}

#include "./msg_file_subscription_registry.hpp"

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

	auto const i = m_port_activity_subscriptions.insert(
		std::pair{
			id,
			msg_file_subscription_handle{
				.port = &port->second,
				.subscriber = subscriber,
				.status = msg_file_input_port_status::ready
			}
		}
	).first;
	utils::maybe_at_scope_exit rollback_subscription{
		[this, i]() noexcept{
			m_port_activity_subscriptions.erase(i);
		}
	};

	port->second.subscriptions.push_back(
		msg_file_output_port_subscription{
			.subscriber = subscriber,
			.id = id
		}
	);
	utils::maybe_at_scope_exit rollback_subscriptions{
		[&port_data = port->second]() noexcept {
			port_data.subscriptions.pop_back();
			port_data.barrier.dec_num_subscribers_without_submitting_result();
		}
	};

	port->second.barrier.inc_num_subscribers();
	port->second.barrier.inc_num_ready_subscribers();

	rollback_subscriptions.reset();
	rollback_subscription.reset();
	rollback_id.reset();

	return id;
}

void Pipe::worker_fwk::msg_file_subscription_registry::notify_client_ready(
	worker_sync::port_activity_subscription_id id,
	port_activity_subscriber_ref subscriber
) const
{
	auto const i = std::as_const(m_port_activity_subscriptions).find(id);
	if(i == std::end(m_port_activity_subscriptions))
	{ throw std::runtime_error{"Invalid subscription id"}; }

	auto& item = i->second;
	if(item.subscriber != subscriber)
	{ throw std::runtime_error{"Invalid subscription id"}; }

	if(item.status == msg_file_input_port_status::ready)
	{ throw std::runtime_error{"Port activity subscriber is already ready"}; }

	item.port->barrier.inc_num_ready_subscribers();
}

void Pipe::worker_fwk::msg_file_subscription_registry::notify_data_ready(port_id id)
{
	auto const port = std::as_const(m_msg_file_output_ports).find(id);
	assert(port != std::end(m_msg_file_output_ports));
	for(auto const& item : port->second.subscriptions)
	{
		auto const subscription = m_port_activity_subscriptions.find(item.id);
		assert(subscription != std::end(m_port_activity_subscriptions));
		assert(subscription->second.status == msg_file_input_port_status::ready);
		item.subscriber.notify_data_ready(item.id);
		subscription->second.status = msg_file_input_port_status::busy;
	}
}

void Pipe::worker_fwk::msg_file_subscription_registry::remove_port_activity_subscription(
	worker_sync::port_activity_subscription_id id,
	port_activity_subscriber_ref subscriber
) const
{
	auto const i = m_port_activity_subscriptions.find(id);

	if(i == std::end(m_port_activity_subscriptions) || i->second.subscriber != subscriber)
	{ throw std::runtime_error{"Invalid subscription id"}; }

	if(i->second.status == Pipe::worker_fwk::msg_file_input_port_status::busy)
	{ i->second.port->barrier.dec_num_subscribers(); }
	else
	{ i->second.port->barrier.dec_num_subscribers_without_submitting_result(); }

	std::erase_if(
		i->second.port->subscriptions,
		[id](auto const& item){
			return item.id == id;
		}
	);
}

void Pipe::worker_fwk::msg_file_subscription_registry::remove_port_activity_subscriber(
	port_activity_subscriber_ref subscriber
)
{
	for(auto const& item : m_msg_file_output_ports)
	{
		std::erase_if(
			item.second.subscriptions,
			[subscriber](auto const& item){
				return item.subscriber == subscriber;
			}
		);
	}

	std::erase_if(
		m_port_activity_subscriptions,
		[subscriber](auto const& item){
			return item.second.subscriber == subscriber;
		}
	);
}