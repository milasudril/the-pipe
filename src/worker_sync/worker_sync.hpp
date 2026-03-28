#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <string>

namespace Pipe::worker_sync
{
	struct port_activity_subscription
	{
		std::string remote_portname;
	};

	struct port_activity_unsubscription
	{
		std::string remote_portname;
	}

	struct data_ready_event
	{
		std::string local_portname;
	};

	struct client_ready_event
	{
		std::string local_portname;
	};
}

#endif