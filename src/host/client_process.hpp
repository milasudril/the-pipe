#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_monitor.hpp"

namespace Pipe::host
{
	class client_process
	{
	public:
		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::ipc::connected_socket_ref<SOCK_STREAM, sockaddr_un>
		)
		{
			if(can_read(event.get_activity_status()))
			{
				// TODO: Decode log entries and dispatch to listener
			}
		}
	};
}