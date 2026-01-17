#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"

namespace prog::host
{
	class client_process
	{
	public:
		explicit client_process(std::file_system::path const& client_binary):
			m_handle{
				os_services::proc_mgmt::spawn(
					client_binary.c_str(),
					std::span<char const*>{},
					std::span<char const*>{},
					os_services::proc_mgmt::io_redirection{
						.sysin = m_host_to_client_handshake_pipe.read_end(),
						.sysout = m_client_to_host_handshake_pipe.write_end(),
						.syserr = {}
					}
				)
			}
		{
		}


	private:
		os_services::ipc::pipe m_host_to_client_handshake_pipe;
		os_services::ipc::pipe m_client_to_host_handshake_pipe;
		os_services::proc_mgmt::pidfd m_handle;
		os_services::ipc::connected_socket<SOCK_STREAM, sockaddr_un> m_control_socket;
	};
}