#include "./client_process.hpp"

#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/fd/activity_monitor.hpp"
#include "src/handshaking_protocol/handshaking_protocol.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/utils/utils.hpp"

#include <random>
#include <unordered_map>

namespace prog::host
{
	class client_activity_handler
	{
	public:
		client_activity_handler(std::reference_wrapper<client_process> client_proc):
			m_client_proc{client_proc}
		{}

		void handle_event(
			os_services::fd::activity_event const&,
			os_services::io::output_file_descriptor_ref
		)
		{
		}

		void handle_event(
			os_services::fd::activity_event const&,
			os_services::io::input_file_descriptor_ref
		)
		{
		}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::proc_mgmt::pidfd_ref pid
		)
		{
			if(can_read(event.get_activity_status()))
			{
				wait(pid);
				event.stop_listening();
			}
		}

	private:
		std::reference_wrapper<client_process> m_client_proc;
	};

	class client_process_repository:std::unordered_map<pid_t, std::unique_ptr<client_process>>
	{
	public:
		using base = std::unordered_map<pid_t, std::unique_ptr<client_process>>;
		using base::find;
		using base::contains;
		using base::begin;
		using base::end;
		using base::size;

		client_process& load(
			std::filesystem::path const& client_binary,
			os_services::fd::activity_monitor& activity_monitor
		)
		{
			os_services::ipc::pipe server_to_client_handshake_pipe;
			os_services::ipc::pipe client_to_server_handshake_pipe;
			auto process = os_services::proc_mgmt::spawn(
				client_binary.c_str(),
				std::span<char const*>{},
				std::span<char const*>{},
				os_services::proc_mgmt::io_redirection{
					.sysin = server_to_client_handshake_pipe.read_end(),
					.sysout = server_to_client_handshake_pipe.write_end(),
					.syserr = {}
				}
			);

			auto client_proc = std::make_unique<client_process>();

			activity_monitor.add(
				std::move(process.second),
				os_services::fd::activity_status::read,
				client_activity_handler{*client_proc}
			);

			activity_monitor.add(
				server_to_client_handshake_pipe.take_write_end(),
				os_services::fd::activity_status::write,
				client_activity_handler{*client_proc}
			);

			activity_monitor.add(
				client_to_server_handshake_pipe.take_read_end(),
				os_services::fd::activity_status::read,
				client_activity_handler{*client_proc}
			);

			assert(emplace(process.first, std::move(client_proc)).second);

			return *client_proc;
		}
	};

	class server_activity_handler
	{
	public:
		explicit server_activity_handler(std::string_view server_name):
			m_server_name{server_name}
		{}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::ipc::server_socket_ref<SOCK_STREAM, sockaddr_un> fd
		)
		{
			if(can_read(event.get_activity_status()))
			{
				m_clients.emplace(m_client_id, accept(fd));
			}
		}

	private:
		std::string m_server_name;
		std::unordered_map<uint64_t, os_services::ipc::connected_socket<SOCK_STREAM, sockaddr_un>> m_clients;
		uint64_t m_client_id{0};
	};

	void make_server(os_services::fd::activity_monitor& activity_monitor)
	{
		auto server_name = utils::random_printable_ascii_string(
			std::tuple_size_v<handshaking_protocol::server_socket_name>
		);

		activity_monitor.add(
			os_services::ipc::make_server_socket<SOCK_STREAM>(
				os_services::ipc::make_abstract_sockaddr_un(server_name),
					1024
			),
			os_services::fd::activity_status::read,
			server_activity_handler{server_name}
		);
	}
}