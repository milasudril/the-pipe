#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/json_io/reader.hpp"
#include <jopp/parser.hpp>

namespace Pipe::host
{
	class client_process
	{
	public:
		struct client_ctl_tag{};
		struct log_stream_tag{};

		template<class... Args>
		void handle_event(os_services::fd::activity_event_handler_store&, Args...)
		{
			// TODO: handle registration events
		}

		void handle_event(json_io::container_loaded_event<log_stream_tag>&& event);
		void handle_event(json_io::parser_error_event<log_stream_tag> event);

		void handle_event(
			os_services::fd::activity_event_handler_store&,
			os_services::fd::activity_event<
				client_ctl_tag,
				os_services::ipc::connected_socket_tag<SOCK_STREAM, sockaddr_un>
			> const& event
		)
		{
			if(can_read(event.status))
			{
				// TODO: Decode log entries and dispatch to listener
			}
		}
	};
}