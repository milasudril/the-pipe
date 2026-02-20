#ifndef PIPE_HOST_SERVER_HPP
#define PIPE_HOST_SERVER_HPP

#include "./client_process.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/client_ctl/startup_config.hpp"
#include "src/utils/utils.hpp"

#include <cstdlib>
#include <ctime>
#include <jopp/parser.hpp>
#include <random>
#include <unordered_map>
#include <jopp/serializer.hpp>

namespace Pipe::host
{
	class client_process_repository:std::unordered_map<pid_t, std::shared_ptr<client_process>>
	{
	public:
		using base = std::unordered_map<pid_t, std::shared_ptr<client_process>>;
		using base::find;
		using base::contains;
		using base::begin;
		using base::end;
		using base::size;

		struct proc_mgmt_tag{};

		template<class... Args>
		void handle_event(Args...)
		{
			// TODO: handle registration events
		}

		void handle_event(
			os_services::fd::activity_event<proc_mgmt_tag, os_services::proc_mgmt::pidfd_tag> const&
		)
		{
	// FIXME:
	//		if(can_read(event.status))
	//		{ source.remove(event.event_handler); }
		}

		void load(
			os_services::fd::activity_event_handler_store& activity_event_handler_store,
			std::filesystem::path const& client_binary
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
			std::array fds_to_keep{Pipe::os_services::fd::make_generic_file_descriptor(ctl_sockets.take_socket_b())};

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
			activity_event_handler_store.make_config_transaction()
				.add<json_io::reader::json_stream_tag>(
					json_io::reader{client_process::log_stream_tag{}, client_proc},
					logpipe.take_read_end(),
					os_services::fd::activity_status::read
				)
				.add<client_process::client_ctl_tag>(
					client_proc,
					ctl_sockets.take_socket_a(),
					os_services::fd::activity_status::write
				)
				.add<proc_mgmt_tag>(
					std::ref(*this),
					std::move(process.second),
					os_services::fd::activity_status::read
				)
				.commit();
		}
	};
}

#endif