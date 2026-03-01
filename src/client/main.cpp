//@	{"target":{"name": "main.o"}}

#include "./application.hpp"

#include "src/json_log/writer.hpp"
#include "src/json_io/reader.hpp"
#include "src/json_io/writer.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/io_multiplexer/epoll_instance.hpp"

#include <jopp/types.hpp>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

int main()
{
	signal(SIGPIPE, SIG_IGN);
	std::chrono::system_clock std_system_clock;
	Pipe::json_log::writer log_writer;

	Pipe::log::context log_ctxt{
		Pipe::log::configuration{
			.writer = std::ref(log_writer),
			.timestamp_generator = std::ref(std_system_clock)
		}
	};

	try
	{
		std::shared_ptr<Pipe::client::application> app = Pipe::client::application::create();
		Pipe::os_services::io_multiplexer::epoll_instance fd_activity_monitor{};

		fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
		fcntl(STDOUT_FILENO, F_SETFL, O_NONBLOCK);

		std::ignore = fd_activity_monitor.add<Pipe::json_io::reader::json_stream_tag>(
			Pipe::json_io::reader{Pipe::client::application::ctl_request_tag{}, app},
			Pipe::os_services::io::input_file_descriptor{
				Pipe::os_services::io::input_file_descriptor_ref{STDIN_FILENO}
			},
			Pipe::os_services::fd::activity_status::read
		);
		std::ignore = fd_activity_monitor.add<Pipe::json_io::writer::json_stream_tag>(
			std::ref(app->get_ctl_output()),
			Pipe::os_services::io::output_file_descriptor{
				Pipe::os_services::io::output_file_descriptor_ref{STDOUT_FILENO}
			},
			Pipe::os_services::fd::activity_status::none
		);

		while(!app->should_exit())
		{
			fd_activity_monitor.wait_for_and_distpatch_events();
		}
	}
	catch(std::exception const& err)
	{
		write_message(Pipe::log::item::severity::error, err.what());
		return -1;
	}

	write_message(Pipe::log::item::severity::info, "Process exited normally");

	return 0;
}