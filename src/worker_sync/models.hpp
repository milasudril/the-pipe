#ifndef PIPE_WORKER_SYNC_MODELS_HPP
#define PIPE_WORKER_SYNC_MODELS_HPP

#include "./worker_sync_msg.hpp"
#include "src/utils/unwrap.hpp"

#include <concepts>

namespace Pipe::worker_sync
{
	template<class T>
	concept output_port_activity_subscription_model = requires(T& obj)
	{
		{ obj.notify_data_ready() } -> std::same_as<void>;
	};

	template<class T, class U>
	concept output_port_model =
		   output_port_activity_subscription_model<decltype(utils::unwrap(std::declval<U>()))>
		&& requires(T& obj,U subscriber)
	{
		{ obj.dec_num_ready_subscribers(subscriber) } -> std::same_as<void>;
		{ obj.add_subscriber(subscriber) } -> std::same_as<void>;
		{ obj.remove_subscriber(subscriber) } -> std::same_as<void>;
	};
}

#endif