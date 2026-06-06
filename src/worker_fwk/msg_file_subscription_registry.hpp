//@	{"dependencies_extra":[{"ref":"./msg_file_subscription_registry.o", "rel":"implementation"}]}

#ifndef PIPE_WORKER_FWK_MSG_FILE_SUBSCRIPTION_REGISTRY_HPP
#define PIPE_WORKER_FWK_MSG_FILE_SUBSCRIPTION_REGISTRY_HPP

#include "./port_activity_subscription.hpp"
#include "src/utils/bound_member_function.hpp"
#include "src/utils/scope_handling.hpp"
#include "src/utils/utils.hpp"
#include "src/utils/unwrap.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <algorithm>
#include <flat_map>
#include <ranges>
#include <string>
#include <memory>
#include <cassert>

namespace Pipe::worker_fwk
{
	class msg_file_output_port_subscription
	{
		port_activity_subscriber_ref subscriber;
		msg_file_output_port* output_port;
		worker_sync::port_activity_subscription_id id;
	};


	enum class msg_file_input_port_status:int{ready, busy};

	struct msg_file_subscription_handle
	{
		msg_file_output_port* port;
		port_activity_subscriber_ref subscriber;
		msg_file_input_port_status status;
	};

	template<class T>
	concept msg_file_output_port_collection = requires(T& obj, std::string const& port_name){
		{ obj.get_msg_file_output_ports() } ->
				utils::bidir_range_of_pairs<port_id, msg_file_subscriber_barrier>;
		{ obj.get_port_id(port_name) } -> std::same_as<port_id>;
	};

#if 0
	class msg_file_subscription_registry
	{
	public:
		template<msg_file_output_port_collection PortCollection>
		msg_file_subscription_registry(PortCollection& output_ports):
			m_ports{&output_ports},
			m_get_port_id{
				[](void* handle, std::string const& port_name){
					return static_cast<PortCollection*>(handle)->get_port_id(port_name);
				}
			},
			m_msg_file_output_ports{
				std::from_range_t{},
				std::ranges::transform_view{
					output_ports.get_msg_file_output_ports(),
					[](auto const& item){
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
		);

		void notify_client_ready(
			worker_sync::port_activity_subscription_id id,
			port_activity_subscriber_ref subscriber
		) const;

		void notify_data_ready(port_id id);

		void remove_port_activity_subscription(
			worker_sync::port_activity_subscription_id id,
			port_activity_subscriber_ref subscriber
		) const;

		void remove_port_activity_subscriber(port_activity_subscriber_ref subscriber);

		auto const& get_msg_file_output_ports() const
		{ return m_msg_file_output_ports; }

		auto const& get_port_acivity_subscriptions() const
		{ return m_port_activity_subscriptions; }

		auto get_current_subscription_id() const
		{ return m_current_subscription_id; }

	private:
		void* m_ports;
		port_id (*m_get_port_id)(void*, std::string const&);
		std::flat_map<port_id, msg_file_output_port> m_msg_file_output_ports;
		std::flat_map<
			worker_sync::port_activity_subscription_id,
			msg_file_subscription_handle
		> m_port_activity_subscriptions;
		worker_sync::port_activity_subscription_id m_current_subscription_id;
	};
#endif
}
#endif