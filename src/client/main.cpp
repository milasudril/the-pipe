//@	{"target":{"name": "main.o"}}

#include "src/json_log/writer.hpp"
#include "src/json_io/reader.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/io_multiplexer/epoll_instance.hpp"

#include <unistd.h>

namespace Pipe::client
{
	class application
	{
	public:
		struct ctl_request_tag{};

		void handle_event(json_io::container_loaded_event<ctl_request_tag>&&)
		{}

		void handle_event(json_io::parser_error_event<ctl_request_tag>)
		{}

	private:
	};
}

int main()
{
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
		auto app = std::make_shared<Pipe::client::application>();
		Pipe::os_services::io_multiplexer::epoll_instance fd_activity_monitor{};
		std::ignore = fd_activity_monitor.add<Pipe::json_io::reader::json_stream_tag>(
			Pipe::json_io::reader{Pipe::client::application::ctl_request_tag{}, app},
			Pipe::os_services::io::input_file_descriptor{
				Pipe::os_services::io::input_file_descriptor_ref{STDIN_FILENO}
			},
			Pipe::os_services::fd::activity_status::read
		);

		while(!fd_activity_monitor.is_empty())
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