#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_monitor.hpp"

namespace Pipe::host
{
	class client_process
	{
	public:
		struct client_ctl_tag{};

		void handle_event(
			os_services::fd::activity_monitor&,
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