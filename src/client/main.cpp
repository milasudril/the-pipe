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

		jopp::container output_object;
		jopp::parser parser{output_object};
		auto const result = parser.parse(std::string_view{argv[1]});
		if(result.ec != jopp::parser_error_code::completed)
		{
			throw std::runtime_error{
				std::format(
					"Failed to parse command line argument: {}",
					to_string(result.ec)
				)
			};
		};

		auto const startup_config = Pipe::client_ctl::make_startup_config(output_object.get<jopp::object>());
	}
	catch(std::exception const& err)
	{
		write_message(Pipe::log::item::severity::error, err.what());
		return -1;
	}

	return 0;
}