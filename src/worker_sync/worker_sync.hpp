#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <string>
#include <cstdint>
#include <variant>

namespace Pipe::worker_sync
{
	struct port_activity_subscription_request
	{ std::string server_portname; };

	struct port_activity_unsubscription
	{ uint64_t subscription_id; };

	struct client_ready_event
	{ uint64_t subscription_id; };

	using client_to_server_message = std::variant<
		port_activity_subscription_request,
		port_activity_unsubscription,
		client_ready_event
	>;

	struct port_activity_subscription
	{ uint64_t subscription_id; };

	struct port_activity_unsubscription_response
	{};

	struct data_ready_event
	{ uint64_t subscription_id; };

	using server_to_client_message = std::variant<
		port_activity_subscription,
		port_activity_unsubscription_response,
		data_ready_event
	>;
}

#endif