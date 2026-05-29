//@	{"dependencies_extra":[{"ref":"./msg_file_subscription_registry.o", "rel":"implementation"}]}

#ifndef PIPE_WORKER_FWK_MSG_FILE_SUBSCRIPTION_REGISTRY_HPP
#define PIPE_WORKER_FWK_MSG_FILE_SUBSCRIPTION_REGISTRY_HPP

#include "./port_activity_subscription.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/worker_fwk/msg_file_subscriber_barrier.hpp"
#include "src/worker_sync/worker_sync.hpp"

#include <algorithm>
#include <flat_map>
#include <ranges>
#include <string>
#include <memory>
#include <cassert>

namespace Pipe::worker_fwk
{
	struct msg_file_output_port_subscription
	{
		port_activity_subscriber_ref subscriber;
		worker_sync::port_activity_subscription_id id;
	};

	struct msg_file_output_port
	{
		msg_file_subcriber_barrier barrier;
		std::vector<msg_file_output_port_subscription> subscriptions;
	};

	enum class msg_file_input_port_status:int{ready, busy};

	struct msg_file_subscription_handle
	{
		msg_file_output_port* port;
		port_activity_subscriber_ref subscriber;
		msg_file_input_port_status status;
	};


	template<class MsgFileOutputṔortCollection>
	class msg_file_subscription_registry
	{
	public:
		msg_file_subscription_registry(MsgFileOutputṔortCollection& output_ports):
			m_ports{output_ports},
			m_msg_file_output_ports{
				std::ranges::transform_view{
					output_ports.get_msg_file_output_ports(),
					[](std::pair<port_id, msg_file_subcriber_barrier> const& item){
						return std::pair{
							item.first,
							msg_file_output_port{
								.barrier = item.second,
								.subscriptions = {}
							}
						};
					}
				}
			}
		{}

		worker_sync::port_activity_subscription_id add_port_activity_subscription(
			std::string const& port_name,
			port_activity_subscriber_ref subscriber
		)
		{
			auto const port = [&](){
				auto const id = m_ports.get_port_id(port_name);
				auto const port = m_msg_file_output_ports.find(id);
				if(port == std::end(m_msg_file_output_ports))
				{ throw std::runtime_error{"The given port is not providing a message file"}; }
				return port;
			};

			auto const id = m_current_subscription_id.next();
			utils::maybe_at_scope_exit rollback_id{
				[this, id](){
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
				[this, i](){
					m_port_activity_subscriptions.erase(i);
				}
			};

			port->second.subscriptions.push_back(
				msg_file_output_port_subscription{
					.subscriber = subscriber,
					.id = id
				}
			);
			rollback_subscription.reset();
			rollback_id.reset();

			return id;
		}

		void notify_client_ready(
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

		void notify_data_ready(port_id id)
		{
			auto const port = std::as_const(m_msg_file_output_ports).find(id);
			assert(port != std::end(m_msg_file_output_ports));
			for(auto const& item : port->second.subscriptions)
			{
				item.subscriber.notify_data_ready(item.id);
				auto const subscription = m_port_activity_subscriptions.find(item.id);
				assert(subscription != std::end(m_port_activity_subscriptions));
				subscription->second.status = msg_file_input_port_status::busy;
			}
		}

		void remove_port_activity_subscription(
			worker_sync::port_activity_subscription_id id,
			port_activity_subscriber_ref subscriber
		) const
		{
			auto const i = m_port_activity_subscriptions.find(id);

			if(i == std::end(m_port_activity_subscriptions) || i->second.subscriber != subscriber)
			{ throw std::runtime_error{"Invalid subscription id"}; }

			i->second.port->barrier.dec_num_subscribers();
			std::erase_if(
				i->second.port->subscriptions,
				[id](auto const& item){
					item.id == id;
				}
			);
		}

		void remove_port_activity_subscriber(port_activity_subscriber_ref subscriber)
		{
			for(auto const& item : m_msg_file_output_ports)
			{
				std::erase_if(
					item.second.subscriptions,
					[subscriber](auto const& item){
						return item.second.subscriber == subscriber;
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

		auto const& get_msg_file_output_ports() const
		{ return m_msg_file_output_ports; }

		auto const& get_port_acivity_subscriptions() const
		{ return m_port_activity_subscriptions; }

		auto get_current_subscription_id() const
		{ return m_current_subscription_id; }

	private:
		MsgFileOutputṔortCollection& m_ports;
		std::flat_map<port_id, msg_file_output_port> m_msg_file_output_ports;
		std::flat_map<
			worker_sync::port_activity_subscription_id,
			msg_file_subscription_handle
		> m_port_activity_subscriptions;
		worker_sync::port_activity_subscription_id m_current_subscription_id;
	};
}
#endif