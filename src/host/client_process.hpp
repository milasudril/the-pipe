#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/handshaking_protocol/handshaking_protocol.hpp"
#include "src/utils/utils.hpp"

#include <filesystem>

namespace prog::host
{
	class client_process
	{
	public:
		explicit client_process(
			handshaking_protocol::server_socket_name const& socket_name,
			std::filesystem::path const& client_binary
		):
			m_process{
				client_binary.c_str(),
				std::span<char const*>{},
				std::span<char const*>{},
				os_services::proc_mgmt::io_redirection{
					.sysin = m_host_to_client_handshake_pipe.read_end(),
					.sysout = m_client_to_host_handshake_pipe.write_end(),
					.syserr = {}
				}
			}
		{
			auto const my_key = utils::random_bytes(std::tuple_size_v<handshaking_protocol::handshake_key>);
			os_services::io::write(
				m_host_to_client_handshake_pipe.write_end(),
				my_key
			);
			handshaking_protocol::handshake_key client_key{};
			os_services::io::read(
				m_client_to_host_handshake_pipe.read_end(),
				client_key
			);
			os_services::io::write(
				m_host_to_client_handshake_pipe.write_end(),
				std::as_bytes(std::span{socket_name})
			);
		}


	private:
		os_services::ipc::pipe m_host_to_client_handshake_pipe;
		os_services::ipc::pipe m_client_to_host_handshake_pipe;
		os_services::proc_mgmt::process m_process;
		os_services::ipc::connected_socket<SOCK_STREAM, sockaddr_un> m_control_socket;
	};
}