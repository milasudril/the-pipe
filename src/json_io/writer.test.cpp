//@	{"target":{"name": "writer.test"}}

#include "./writer.hpp"

#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/ipc/pipe.hpp"
#include "testfwk/validation.hpp"

#include <fcntl.h>
#include <testfwk/testfwk.hpp>
#include <signal.h>

namespace
{
	class my_activity_event_handler_store:public Pipe::os_services::fd::activity_event_handler_store
	{
	public:
		std::optional<Pipe::os_services::fd::activity_status> expected_new_listening_status;
		std::optional<Pipe::os_services::fd::event_handler_id> expected_remove_id;

		Pipe::os_services::error_handling::code update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref,
			Pipe::os_services::fd::activity_status status
		) noexcept override
		{
			EXPECT_EQ(status, expected_new_listening_status);
			expected_new_listening_status.reset();
			// TODO: Should try different error code?
			return Pipe::os_services::error_handling::code{};
		}

	private:
		void remove(Pipe::os_services::fd::event_handler_id id) noexcept override
		{
			EXPECT_EQ(id, expected_remove_id);
			expected_remove_id.reset();
		}

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{ throw std::runtime_error{"Unexpected call do_update_listening_status"}; }
	};
}

TESTCASE(Pipe_json_io_writer_buffer_full_before_queue_empty_no_pending_data_to_write_io_completed)
{
	Pipe::json_io::writer writer{16};

	my_activity_event_handler_store eh_store;
	Pipe::os_services::ipc::pipe io_channel;
	writer.handle_event(
		Pipe::json_io::writer::activity_event_handler_registered_event{
			.fd = io_channel.write_end(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_store
		}
	);
	fcntl(io_channel.read_end().native_handle(), F_SETFL, O_NONBLOCK);

	jopp::object stuff_to_write{};
	stuff_to_write.insert("my_property", "This will be longer than 16 bytes");
	writer.write(jopp::container{std::move(stuff_to_write)});

	EXPECT_EQ(writer.get_reminder_size(), 0);

	std::array<char, 16> bytes_written{};
	auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{bytes_written}));
	EXPECT_EQ(result.bytes_transferred(), 16);
	EXPECT_EQ(result.operation_would_have_blocked(), false);

	std::string_view sv{bytes_written};
	EXPECT_EQ(sv, "{\"my_property\":\"");
}

TESTCASE(Pipe_json_io_writer_buffer_full_before_queue_empty_no_pending_data_to_write_io_blocked_and_trigger_again_until_completed)
{
	Pipe::json_io::writer writer{6144};

	my_activity_event_handler_store eh_store;
	Pipe::os_services::ipc::pipe io_channel;
	writer.handle_event(
		Pipe::json_io::writer::activity_event_handler_registered_event{
			.fd = io_channel.write_end(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_store
		}
	);

	// Set pipe size and mark file descriptors as non-blocking
	fcntl(io_channel.write_end(), F_SETPIPE_SZ, 8192);
	fcntl(io_channel.write_end(), F_SETFL, O_NONBLOCK);
	fcntl(io_channel.read_end(), F_SETFL, O_NONBLOCK);

	// First fill the pipe to trigger a partial write
	{
		std::array<std::byte, 4096> initial_data{};
		auto const result = write(io_channel.write_end(), initial_data);
		EXPECT_EQ(result.bytes_transferred(), std::size(initial_data));
		EXPECT_EQ(result.operation_would_have_blocked(), false);
	}

	// Prepare the data
	jopp::object stuff_to_write{};
	std::string a_long_string;
	for(size_t k = 0; k != 16384; ++k)
	{ a_long_string += static_cast<char>(65 + k%26); }
	stuff_to_write.insert("my_property", std::move(a_long_string));
	auto const expected_data = to_string(stuff_to_write);
	std::string received_data;

	// First write will trigger state change, since the I/O operation would have blocked
	eh_store.expected_new_listening_status = Pipe::os_services::fd::activity_status::write;
	writer.write(jopp::container{std::move(stuff_to_write)});
	EXPECT_EQ(writer.get_reminder_size(), 2048);

	// Try to read again will fail since operation is still blocked
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
	EXPECT_EQ(writer.get_reminder_size(), 2048);

	// Read the extra data written to fill up the pipe
	{
		std::array<std::byte, 4096> buffer{};
		auto const result = read(io_channel.read_end(), buffer);
		EXPECT_EQ(result.bytes_transferred(), std::size(buffer));
		EXPECT_EQ(result.operation_would_have_blocked(), false);
	}

	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 4096);
		EXPECT_EQ(result.operation_would_have_blocked(), false);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	// Now it should be possible to put more data into the pipe. Since we have read all content of
	// the pipe, and the buffer size is less than the pipe, there should be no remaining bytes left.
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
	EXPECT_EQ(writer.get_reminder_size(), 0);

	// Read more data
	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 4096);
		EXPECT_EQ(result.operation_would_have_blocked(), false);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	// Keep pumping data
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
	EXPECT_EQ(writer.get_reminder_size(), 19);

	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 4096);
		EXPECT_EQ(result.operation_would_have_blocked(), false);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	// Now, we have processed all data
	eh_store.expected_new_listening_status = Pipe::os_services::fd::activity_status::none;
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
	EXPECT_EQ(writer.get_reminder_size(), 0);
	EXPECT_EQ(writer.serialization_queue_is_empty(), true);

	// Nothing happens when queue is empty
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
	EXPECT_EQ(writer.get_reminder_size(), 0);

	// Fetch remaining data
	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 4096);
		EXPECT_EQ(result.operation_would_have_blocked(), false);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 19);
		EXPECT_EQ(result.operation_would_have_blocked(), false);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	{
		std::array<char, 4096> buffer{};
		auto const result = read(io_channel.read_end(), std::as_writable_bytes(std::span{buffer}));
		EXPECT_EQ(result.bytes_transferred(), 0);
		EXPECT_EQ(result.operation_would_have_blocked(), true);
		received_data.insert(
			std::end(received_data),
			std::begin(buffer), std::begin(buffer) + result.bytes_transferred()
		);
	}

	EXPECT_EQ(received_data, expected_data);
}

TESTCASE(Pipe_json_io_writer_remote_fd_closed_when_trying_to_write_reminder)
{
	signal(SIGPIPE, SIG_IGN);
	Pipe::json_io::writer writer{6144};

	my_activity_event_handler_store eh_store;
	Pipe::os_services::ipc::pipe io_channel;
	writer.handle_event(
		Pipe::json_io::writer::activity_event_handler_registered_event{
			.fd = io_channel.write_end(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_store
		}
	);

	// Set pipe size and mark file descriptors as non-blocking
	fcntl(io_channel.write_end(), F_SETPIPE_SZ, 8192);
	fcntl(io_channel.write_end(), F_SETFL, O_NONBLOCK);
	fcntl(io_channel.read_end(), F_SETFL, O_NONBLOCK);

	// First fill the pipe to trigger a partial write
	{
		std::array<std::byte, 4096> initial_data{};
		auto const result = write(io_channel.write_end(), initial_data);
		EXPECT_EQ(result.bytes_transferred(), std::size(initial_data));
		EXPECT_EQ(result.operation_would_have_blocked(), false);
	}

	// Prepare the data
	jopp::object stuff_to_write{};
	std::string a_long_string;
	for(size_t k = 0; k != 16384; ++k)
	{ a_long_string += static_cast<char>(65 + k%26); }
	stuff_to_write.insert("my_property", std::move(a_long_string));
	auto const expected_data = to_string(stuff_to_write);
	std::string received_data;

	// First write will trigger state change, since the I/O operation would have blocked
	eh_store.expected_new_listening_status = Pipe::os_services::fd::activity_status::write;
	writer.write(jopp::container{std::move(stuff_to_write)});
	EXPECT_EQ(writer.get_reminder_size(), 2048);

	io_channel.close_read_end();
	eh_store.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	writer.handle_event(Pipe::json_io::writer::fd_ready_event{});
}

TESTCASE(Pipe_json_io_writer_remote_fd_closed_when_trying_to_write_new_data)
{
	signal(SIGPIPE, SIG_IGN);
	Pipe::json_io::writer writer{16};

	my_activity_event_handler_store eh_store;
	Pipe::os_services::ipc::pipe io_channel;
	writer.handle_event(
		Pipe::json_io::writer::activity_event_handler_registered_event{
			.fd = io_channel.write_end(),
			.id = Pipe::os_services::fd::event_handler_id{345},
			.event_handler = {},
			.event_handler_store = &eh_store
		}
	);
	fcntl(io_channel.read_end().native_handle(), F_SETFL, O_NONBLOCK);

	jopp::object stuff_to_write{};
	stuff_to_write.insert("my_property", "This will be longer than 16 bytes");
	io_channel.close_read_end();
	eh_store.expected_remove_id = Pipe::os_services::fd::event_handler_id{345};
	writer.write(jopp::container{std::move(stuff_to_write)});
}