//@	{"target":{"name": "main.o"}}

#include "src/json_log/writer.hpp"
#include "src/log/log.hpp"
#include "src/client_ctl/startup_config.hpp"

#include <cstdio>

int main(int argc, char**)
{
	std::chrono::system_clock std_system_clock;
	Pipe::json_log::writer log_writer;

	Pipe::log::context log_ctxt{
		Pipe::log::configuration{
			.writer = std::ref(log_writer),
			.timestamp_generator = std::ref(std_system_clock)
		}
	};

	if(argc != 2)
	{
		write_message(
			Pipe::log::severity::error,
			"Wrong number of command line arguments. Got {} expected 2",
			argc
		);
		return -1;
	}
}