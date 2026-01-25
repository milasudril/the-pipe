//@	{"target":{"name":"writer.test"}}

#include "./writer.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_json_log_writer_write_message)
{
	Pipe::json_log_writer::writer writer{};
	writer.write(Pipe::log::item{});
}