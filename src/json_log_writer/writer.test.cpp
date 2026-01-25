//@	{"target":{"name":"writer.test"}}

#include "./writer.hpp"
#include "src/log/log.hpp"
#include "src/os_services/io/io.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_log_writer_write_message)
{
	Pipe::json_log_writer::writer writer{};
	writer.write(Pipe::log::item{
		.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{1},
		.severity = Pipe::log::severity::info,
		.message = "Hello, world"
	});

	Pipe::os_services::io::output_file_descriptor_ref fd{STDOUT_FILENO};
	auto const result = writer.pump_data(fd);
	EXPECT_EQ(result, Pipe::json_log_writer::writer::flush_result::keep_going);

}