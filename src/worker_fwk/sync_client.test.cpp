//@	{"target":{"name":"sync_client.test"}}

#include "./sync_client.hpp"
#include "src/worker_sync/worker_sync_msg.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct input_port_activity_subscriber
	{
		void sync_client_lost_connection_to_server(void const*)
		{}

		void notify_data_ready(Pipe::worker_sync::port_activity_subscription_id)
		{}

		void subscription_completed(
			Pipe::worker_sync::transaction_id,
			Pipe::worker_sync::port_activity_subscription_id
		)
		{}

		void unsubscription_completed(Pipe::worker_sync::transaction_id)
		{}
	};

};

TESTCASE(Pipe_worker_fwk_sync_client_construct)
{
	input_port_activity_subscriber subscriber;
	Pipe::worker_fwk::sync_client client{std::ref(subscriber)};
}