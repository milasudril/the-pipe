//@	{"target":{"name":"writer.test"}}

#include "./writer.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/io/io.hpp"

#include <sys/mman.h>
#include <testfwk/testfwk.hpp>
#include <jopp/parser.hpp>
#include <unistd.h>

static_assert(
	Pipe::os_services::fd::activity_event_handler<
		Pipe::json_log_writer::writer,
		Pipe::os_services::io::output_file_descriptor_tag
	>
);

TESTCASE(Pipe_json_log_writer_direct_write_small_buffer)
{
	Pipe::os_services::fd::file_descriptor fd{memfd_create("", 0)};

	Pipe::json_log_writer::writer writer{
		5,
		Pipe::os_services::io::output_file_descriptor_ref{fd.get().native_handle()}
	};
	writer.write(Pipe::log::item{
		.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{1},
		.severity = Pipe::log::severity::info,
		.message = "First message"
	});

	writer.write(Pipe::log::item{
		.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{2},
		.severity = Pipe::log::severity::warning,
		.message = "Second message"
	});

	lseek(fd.get(), 0, SEEK_SET);

	std::array<char, 4096>  result_buffer{};
	auto const res = read(
		Pipe::os_services::io::input_file_descriptor_ref{fd.get().native_handle()},
		std::as_writable_bytes(std::span{result_buffer})
	);
	EXPECT_GT(res.bytes_transferred(), 0);

	std::span remaining{std::data(result_buffer), res.bytes_transferred()};

	{
		jopp::container root;
		jopp::parser json_parser{root};
		auto parse_result = json_parser.parse(remaining);
		EXPECT_EQ(parse_result.ec, jopp::parser_error_code::completed);
		EXPECT_NE(parse_result.ptr, std::begin(remaining));
		remaining = std::span{parse_result.ptr, std::end(remaining)};
		auto const root_value = root.get_if<jopp::object>();
		REQUIRE_NE(root_value, nullptr);
		auto const item = Pipe::json_log_writer::make_log_item(*root_value);

		EXPECT_EQ(
			item,
			(Pipe::log::item{
				.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{1},
				.severity = Pipe::log::severity::info,
				.message = "First message"
			})
		);
	}

	{
		jopp::container root;
		jopp::parser json_parser{root};
		auto parse_result = json_parser.parse(remaining);
		EXPECT_EQ(parse_result.ec, jopp::parser_error_code::completed);
		EXPECT_NE(parse_result.ptr, std::begin(remaining));
		remaining = std::span{parse_result.ptr, std::end(remaining)};
		auto const root_value = root.get_if<jopp::object>();
		REQUIRE_NE(root_value, nullptr);
		auto const item = Pipe::json_log_writer::make_log_item(*root_value);

		EXPECT_EQ(
			item,
			(Pipe::log::item{
				.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{2},
				.severity = Pipe::log::severity::warning,
				.message = "Second message"
			})
		);
	}
}

namespace
{
	struct my_activity_event:public Pipe::os_services::fd::activity_event
	{
		mutable Pipe::os_services::fd::activity_status current_activity_status;
		mutable bool listening_stopped = false;

		Pipe::os_services::fd::activity_status get_activity_status() const noexcept override
		{ return current_activity_status; }

		void stop_listening() const noexcept override
		{
			listening_stopped = true;
		}

		void update_listening_status(Pipe::os_services::fd::activity_status new_status)
		const noexcept override
		{
			current_activity_status = new_status;
		}
	};
};

TESTCASE(Pipe_json_log_writer_write_with_fd_set_delays_write)
{
	Pipe::os_services::fd::file_descriptor fd_1{memfd_create("", 0)};

	Pipe::json_log_writer::writer writer{
		65536,
		Pipe::os_services::io::output_file_descriptor_ref{fd_1.get().native_handle()}
	};

	my_activity_event event{};
	event.current_activity_status = Pipe::os_services::fd::activity_status::write;

	Pipe::os_services::fd::file_descriptor fd_2{memfd_create("", 0)};
	writer.handle_event(
		event,
		Pipe::os_services::io::output_file_descriptor_ref{fd_2.get().native_handle()}
	);

	writer.write(Pipe::log::item{
		.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{1},
		.severity = Pipe::log::severity::info,
		.message = "First message"
	});
	lseek(fd_1.get(), 0, SEEK_SET);
	lseek(fd_2.get(), 0, SEEK_SET);

	std::array<char, 4096>  result_buffer{};
	{
		auto const res = read(
			Pipe::os_services::io::input_file_descriptor_ref{fd_1.get().native_handle()},
			std::as_writable_bytes(std::span{result_buffer})
		);
		EXPECT_EQ(res.bytes_transferred(), 0);
	}

	{
		auto const res = read(
			Pipe::os_services::io::input_file_descriptor_ref{fd_2.get().native_handle()},
			std::as_writable_bytes(std::span{result_buffer})
		);
		EXPECT_EQ(res.bytes_transferred(), 0);
	}

	writer.handle_event(
		event,
		Pipe::os_services::io::output_file_descriptor_ref{fd_2.get().native_handle()}
	);

	lseek(fd_1.get(), 0, SEEK_SET);
	lseek(fd_2.get(), 0, SEEK_SET);

	{
		auto const res = read(
			Pipe::os_services::io::input_file_descriptor_ref{fd_1.get().native_handle()},
			std::as_writable_bytes(std::span{result_buffer})
		);
		EXPECT_EQ(res.bytes_transferred(), 0);
	}


	auto const res = read(
		Pipe::os_services::io::input_file_descriptor_ref{fd_2.get().native_handle()},
		std::as_writable_bytes(std::span{result_buffer})
	);
	EXPECT_GT(res.bytes_transferred(), 0);
	std::span remaining{std::data(result_buffer), res.bytes_transferred()};

	{
		jopp::container root;
		jopp::parser json_parser{root};
		auto parse_result = json_parser.parse(remaining);
		EXPECT_EQ(parse_result.ec, jopp::parser_error_code::completed);
		EXPECT_NE(parse_result.ptr, std::begin(remaining));
		remaining = std::span{parse_result.ptr, std::end(remaining)};
		auto const root_value = root.get_if<jopp::object>();
		REQUIRE_NE(root_value, nullptr);
		auto const item = Pipe::json_log_writer::make_log_item(*root_value);

		EXPECT_EQ(
			item,
			(Pipe::log::item{
				.when = std::chrono::system_clock::time_point{} + std::chrono::seconds{1},
				.severity = Pipe::log::severity::info,
				.message = "First message"
			})
		);
	}
}
#if 0
	Pipe::os_services::io::output_file_descriptor_ref fd{STDERR_FILENO};
	auto const result = writer.pump_data(fd);
	EXPECT_EQ(result, Pipe::json_log_writer::writer::flush_result::keep_going);
#endif