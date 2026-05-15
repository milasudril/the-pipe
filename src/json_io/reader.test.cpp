//@	{"target":{"name": "reader.test"}}

#include "./reader.hpp"

#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/ipc/pipe.hpp"

#include <jopp/parser.hpp>
#include <jopp/serializer.hpp>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_receiver
	{
		jopp::container recv_item{};
		jopp::parser_error_code parser_error = jopp::parser_error_code::completed;

		void handle_event(Pipe::json_io::container_loaded_event<void>&& event)
		{
			recv_item = std::move(event.obj);
			parser_error = jopp::parser_error_code::completed;
		}

		void handle_event(Pipe::json_io::input_closed_event<void>)
		{}

		void handle_event(Pipe::json_io::parser_error_event<void> event)
		{ parser_error = event.ec; }
	};

	class my_activity_event_handler_store:public Pipe::os_services::fd::activity_event_handler_store
	{
	public:
		Pipe::os_services::fd::event_handler_id removed_id{};

		void update_listening_status(
			Pipe::os_services::fd::saved_event_handler_ref,
			Pipe::os_services::fd::activity_status
		) noexcept override
		{ Pipe::log::terminate_with_message("Unexpected call update_listening_status"); }

	private:
		void remove(Pipe::os_services::fd::event_handler_id id) noexcept override
		{ removed_id = id; }

		Pipe::os_services::fd::event_handler_id do_add(
			event_handler_info const&,
			Pipe::os_services::fd::file_descriptor,
			Pipe::os_services::fd::activity_status
		) override
		{ throw std::runtime_error{"Unexpected call do_update_listening_status"}; }
	};
}

TESTCASE(Pipe_json_io_reader_cannot_read)
{
	my_receiver receiver;
	my_activity_event_handler_store monitor;
	Pipe::json_io::reader reader{std::ref(receiver), 16};
	Pipe::os_services::ipc::pipe logpipe;
	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	constexpr Pipe::os_services::fd::event_handler_id my_id{2465};
	reader.handle_event(Pipe::json_io::reader::activity_event_handler_registered_event{
		.fd = logpipe.read_end(),
		.id = my_id,
		.event_handler = {},
		.event_handler_store = &monitor
	});

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::none
		}
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_NE(monitor.removed_id, my_id);

	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(receiver.recv_item.empty(), true);
}

TESTCASE(Pipe_json_io_reader_read_full_read_partial_block_close_try_agian)
{
	my_receiver receiver;
	my_activity_event_handler_store monitor;

	Pipe::json_io::reader reader{std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;
	constexpr Pipe::os_services::fd::event_handler_id my_id{2465};
	reader.handle_event(Pipe::json_io::reader::activity_event_handler_registered_event{
		.fd = logpipe.read_end(),
		.id = my_id,
		.event_handler = {},
		.event_handler_store = &monitor
	});

	std::string str{"{\"message\": \"This is the first message\"}\n"};
	str+="{\"message\": \"This is the second message\"}";

	auto const stop_at = (3*std::size(str))/4;

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{std::data(str), stop_at}));

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);
	EXPECT_NE(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	auto const& item = receiver.recv_item.get<jopp::object>();
	EXPECT_EQ(item.get_field_as<std::string>("message"), "This is the first message");
	receiver = my_receiver{};

	logpipe.close_write_end();

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);
	EXPECT_EQ(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::more_data_needed);
	EXPECT_EQ(receiver.recv_item.empty(), true);
}

TESTCASE(Pipe_json_io_reader_read_full_read_partial_block_try_agian_close)
{
	my_receiver receiver;
	my_activity_event_handler_store monitor;

	Pipe::json_io::reader reader{std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;
	constexpr Pipe::os_services::fd::event_handler_id my_id{2465};
	reader.handle_event(Pipe::json_io::reader::activity_event_handler_registered_event{
		.fd = logpipe.read_end(),
		.id = my_id,
		.event_handler = {},
		.event_handler_store = &monitor
	});

	std::string str{"{\"message\": \"This is the first message\"}\n"};
	str+="{\"message\": \"This is the second message\"}";

	auto const stop_at = (3*std::size(str))/4;

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{std::data(str), stop_at}));

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);

	EXPECT_NE(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	{
		auto const& item = receiver.recv_item.get<jopp::object>();
		EXPECT_EQ(item.get_field_as<std::string>("message"), "This is the first message");
	}
	receiver = my_receiver{};

	write(logpipe.write_end(), std::as_bytes(std::span{std::begin(str) + stop_at, std::end(str)}));

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);

	EXPECT_NE(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	{
		auto const& item = receiver.recv_item.get<jopp::object>();
		EXPECT_EQ(item.get_field_as<std::string>("message"), "This is the second message");
	}
	receiver = my_receiver{};

	logpipe.close_write_end();

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);

	EXPECT_EQ(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(receiver.recv_item.empty(), true);
}

TESTCASE(Pipe_json_io_reader_read_jammed_parser)
{
	my_receiver receiver;
	my_activity_event_handler_store monitor;

	Pipe::json_io::reader reader{std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;
	constexpr Pipe::os_services::fd::event_handler_id my_id{2465};
	reader.handle_event(Pipe::json_io::reader::activity_event_handler_registered_event{
		.fd = logpipe.read_end(),
		.id = my_id,
		.event_handler = {},
		.event_handler_store = &monitor
	});
	std::string str{"This is definitely not valid JSON {}"};

	write(logpipe.write_end(), std::as_bytes(std::span{str}));
	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);

	reader.handle_event(
		Pipe::json_io::reader::data_available_event{
			.status = Pipe::os_services::fd::activity_status::read
		}
	);

	EXPECT_EQ(monitor.removed_id, my_id);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::no_top_level_node);
	EXPECT_EQ(receiver.recv_item.empty(), true);
}
