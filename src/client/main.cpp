//@	{"target":{"name": "main.o"}}

#include "src/json_log/writer.hpp"
#include "src/log/log.hpp"
#include "src/client_ctl/startup_config.hpp"

#include <cstdio>
#include <jopp/parser.hpp>
#include <unistd.h>

int main(int argc, char** argv)
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
		if(argc != 2)
		{
			throw std::runtime_error{
				std::format("Wrong number of command line arguments. Got {} expected 2", argc)
			};
		}

		auto const startup_config = Pipe::client_ctl::make_startup_config(
			jopp::parse(std::string_view{argv[1]}).get<jopp::object>()
		);
	}
	catch(std::exception const& err)
	{
		write_message(Pipe::log::item::severity::error, err.what());
		return -1;
	}

	write_message(Pipe::log::item::severity::info, "Process exited normally");

	return 0;
}