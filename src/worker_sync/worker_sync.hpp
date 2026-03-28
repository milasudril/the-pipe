#ifndef PIPE_WORKER_SYNC_WORKER_SYNC_HPP
#define PIPE_WORKER_SYNC_WORKER_SYNC_HPP

#include <pthread.h>
#include <string>
#include <cstdint>

namespace Pipe::worker_sync
{
	struct protocol_revision_request
	{
		std::vector<uint64_t> acceptable_versions;
	};

	struct protocol_revision_response
	{
		uint64_t selected_version;
	};

	namespace v0
	{
		struct port_activity_subscription_request
		{
			std::string server_portname;
		};

		struct port_activity_subscription
		{
			uint64_t subscription_id;
		};

		struct port_activity_unsubscription
		{
			uint64_t subscription_id;
		};

		struct port_activity_unsubscription_response
		{};

		struct data_ready_event
		{
			uint64_t subscription_id;
		};

		struct client_ready_event
		{
			uint64_t subscription_id;
		};

		enum class client_to_server_msg_types:uint32_t{
			protocol_revision_request,
			port_activity_subscription_request,
			port_activity_unsubscription,
			client_ready_event
		};

		struct request_header
		{
			uint64_t transaction_id;
			client_to_server_msg_types msg_type;
			uint32_t msg_size;
		};

		enum class server_to_client_msg_types:uint32_t{
			protocol_revision_response,
			port_activity_subscription,
			port_activity_unsubscription_response,
			data_ready_event
		};

		struct response_header
		{
			uint64_t transaction_id;
			server_to_client_msg_types msg_type;
			uint32_t msg_size;
		};
	}
}

#endif