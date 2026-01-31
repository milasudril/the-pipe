#include "./client_process.hpp"

#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/os_services/fd/activity_monitor.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/client_ctl/startup_config.hpp"
#include "src/utils/utils.hpp"

#include <ctime>
#include <random>
#include <unordered_map>
#include <jopp/serializer.hpp>

namespace Pipe::host
{
	class log_reader
	{
	public:
		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::io::input_file_descriptor_ref
		)
		{
			if(can_read(event.get_activity_status()))
			{
				// TODO: Decode log entries and dispatch to listener
			}
		}
	};

	class client_process_repository:std::unordered_map<pid_t, std::shared_ptr<client_process>>
	{
	public:
		using base = std::unordered_map<pid_t, std::shared_ptr<client_process>>;
		using base::find;
		using base::contains;
		using base::begin;
		using base::end;
		using base::size;

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::proc_mgmt::pidfd_ref
		)
		{
			if(can_read(event.get_activity_status()))
			{
				// TODO: Decode log entries and dispatch to listener
			}
		}

		void load(
			std::filesystem::path const& client_binary,
			os_services::fd::activity_monitor& activity_monitor
		)
		{
			os_services::ipc::pipe logpipe;
			os_services::ipc::socket_pair<SOCK_STREAM> ctl_sockets;
			auto const startup_config = to_string(
				client_ctl::to_jopp_object(
					client_ctl::startup_config{
						client_ctl::host_info{
							.address = ctl_sockets.socket_b()
						}
					}
				)
			);
			std::array args_cstr{startup_config.c_str()};
			std::array fds_to_keep{Pipe::os_services::fd::file_descriptor{ctl_sockets.take_socket_b().release()}};

			auto process = os_services::proc_mgmt::spawn(
				client_binary.c_str(),
				std::span{std::data(args_cstr), 1},
				std::span<char const*>{},
				os_services::proc_mgmt::io_redirection{
					.sysin = {},
					.sysout = {},
					.syserr = logpipe.take_write_end()
				},
				std::span{fds_to_keep}
			);

			auto client_proc = std::make_shared<client_process>();
			activity_monitor.make_config_transaction()
				.add(
					logpipe.take_read_end(),
					os_services::fd::activity_status::read,
					log_reader{}
				)
				.add(
					ctl_sockets.take_socket_a(),
					os_services::fd::activity_status::write,
					client_proc
				)
				.add(
					std::move(process.second),
					os_services::fd::activity_status::read,
					std::ref(*this)
				)
			.commit();
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
}