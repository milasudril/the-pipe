#ifndef PIPE_HOST_SERVER_HPP
#define PIPE_HOST_SERVER_HPP

#include "./worker_process.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "src/os_services/ipc/socket.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"
#include "src/os_services/ipc/socket_pair.hpp"
#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include "src/utils/utils.hpp"

#include <cstdlib>
#include <ctime>
#include <jopp/parser.hpp>
#include <unordered_map>
#include <jopp/serializer.hpp>

namespace Pipe::host
{
	class worker_process_repository:
		public process_manager,
		private std::unordered_map<pid_t, std::shared_ptr<worker_process>>
	{
	public:
		using base = std::unordered_map<pid_t, std::shared_ptr<worker_process>>;
		using base::find;
		using base::contains;
		using base::begin;
		using base::end;
		using base::size;

		void load(
			os_services::fd::activity_event_handler_store& activity_event_handler_store,
			std::filesystem::path&& worker_binary
		)
		{
			os_services::ipc::pipe logpipe;
			os_services::ipc::pipe request_pipe;
			os_services::ipc::pipe response_pipe;

			auto process = os_services::proc_mgmt::spawn(
				worker_binary.c_str(),
				std::span<char const*>{},
				std::span<char const*>{},
				os_services::proc_mgmt::io_redirection{
					.sysin = request_pipe.take_read_end(),
					.sysout = response_pipe.take_write_end(),
					.syserr = logpipe.take_write_end()
				},
				std::span<os_services::fd::file_descriptor>{}
			);

			auto worker_proc = std::make_shared<worker_process>(*this, process.first, std::move(worker_binary));

			auto cfg_transaction = activity_event_handler_store.make_config_transaction()
				.add<json_io::writer::json_stream_tag>(
					std::ref(worker_proc->get_ctl_output()),
					request_pipe.take_write_end(),
					os_services::fd::activity_status::none
				)
				.add<json_io::reader::json_stream_tag>(
					json_io::reader{worker_process::worker_ctl_tag{}, worker_proc},
					response_pipe.take_read_end(),
					os_services::fd::activity_status::read
				)
				.add<json_io::reader::json_stream_tag>(
					json_io::reader{worker_process::log_stream_tag{}, worker_proc},
					logpipe.take_read_end(),
					os_services::fd::activity_status::read
				)
				.add<void>(
					worker_proc,
					std::move(process.second),
					os_services::fd::activity_status::read
				);

			auto const ip = insert(std::pair{process.first, std::move(worker_proc)});
			assert(ip.second);

			cfg_transaction.commit();
		}
	private:
	};
}

#endif