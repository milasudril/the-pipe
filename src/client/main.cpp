//@	{"target":{"name": "main.o"}}

#include "src/json_log/writer.hpp"
#include "src/log/log.hpp"

#include <unistd.h>

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
	}
	catch(std::exception const& err)
	{
		write_message(Pipe::log::item::severity::error, err.what());
		return -1;
	}

	write_message(Pipe::log::item::severity::info, "Process exited normally");

	return 0;
}