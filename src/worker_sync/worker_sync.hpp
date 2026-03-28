#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <string>

namespace Pipe::worker_sync
{
	struct port_activity_subscription
	{
		std::string server_portname;
	};

	struct port_activity_unsubscription
	{
		std::string server_portname;
	}

	struct data_ready_event
	{
		std::string server_portname;
	};

	struct client_ready_event
	{
		std::string server_portname;
	};
}

#endif