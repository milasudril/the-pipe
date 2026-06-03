#ifndef PIPE_WORKER_SYNC_MODELS_HPP
#define PIPE_WORKER_SYNC_MODELS_HPP

#include <concepts>

namespace Pipe::worker_sync
{
	template<class T>
	concept output_port_activity_subscription_model = requires(T& obj)
	{
		{ obj.notify_data_ready() } -> std::same_as<void>;
	};
}

#endif