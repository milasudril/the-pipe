//@	{"target":{"name": "reader.test"}}

#include "./reader.hpp"
#include "./item_converter.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/ipc/pipe.hpp"

#include <jopp/parser.hpp>
#include <jopp/serializer.hpp>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_receiver
	{
		Pipe::log::item recv_item{};
		jopp::parser_error_code parser_error = jopp::parser_error_code::completed;
		std::vector<std::string> errmesg{};

		void consume(char const*, Pipe::log::item&& item)
		{
			recv_item = std::move(item);
			parser_error = jopp::parser_error_code::completed;
		}

		void on_parse_error(char const*, jopp::parser_error_code ec)
		{ parser_error = ec; }

		void on_invalid_log_item(char const*, char const* msg)
		{ errmesg.push_back(msg);}
	};

	struct my_fd_activity_event:public Pipe::os_services::fd::activity_event
	{
		explicit my_fd_activity_event(
			Pipe::os_services::fd::activity_status current_activity_status,
			Pipe::os_services::fd::activity_status* listening_status,
			bool* stop_listening
		):
			m_current_activity_status{current_activity_status},
			m_listening_status{listening_status},
			m_stop_listening{stop_listening}
		{}

		Pipe::os_services::fd::activity_status m_current_activity_status;
		Pipe::os_services::fd::activity_status* m_listening_status;
		bool* m_stop_listening;

		Pipe::os_services::fd::activity_status get_activity_status() const noexcept override
		{ return m_current_activity_status; }

		void update_listening_status(Pipe::os_services::fd::activity_status new_activity_status) const noexcept override
		{
			*m_listening_status = new_activity_status;
		}

		void stop_listening() const noexcept override
		{
			*m_stop_listening = true;
		}
	};
}

TESTCASE(Pipe_json_log_reader_cannot_read)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver), 16};
	Pipe::os_services::ipc::pipe logpipe;

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::write,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);

	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(receiver.recv_item, Pipe::log::item{});
}

TESTCASE(Pipe_json_log_reader_read_full_read_partial_block_close_try_agian)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;

	auto str = to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the first message"
			}
		)
	);

	str += to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the second message"
			}
		)
	);

	auto const stop_at = (3*std::size(str))/4;

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{std::data(str), stop_at}));

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);
	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(
		receiver.recv_item,
		(Pipe::log::item{
			.when = {},
			.severity = {},
			.message = "This is the first message"
		})
	);
	receiver = my_receiver{};

	logpipe.close_write_end();

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);
	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, true);
	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::more_data_needed);
	EXPECT_EQ(receiver.recv_item, Pipe::log::item{});
}

TESTCASE(Pipe_json_log_reader_read_full_read_partial_block_try_agian_close)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;

	auto str = to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the first message"
			}
		)
	);

	str += to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the second message"
			}
		)
	);

	auto const stop_at = (3*std::size(str))/4;

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{std::data(str), stop_at}));

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);
	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(
		receiver.recv_item,
		(Pipe::log::item{
			.when = {},
			.severity = {},
			.message = "This is the first message"
		})
	);
	receiver = my_receiver{};

	write(logpipe.write_end(), std::as_bytes(std::span{std::begin(str) + stop_at, std::end(str)}));

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);
	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(
		receiver.recv_item,
		(Pipe::log::item{
			.when = {},
			.severity = {},
			.message = "This is the second message"
		})
	);
	receiver = my_receiver{};

	logpipe.close_write_end();

	reader.handle_event(
	my_fd_activity_event{
		Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, true);
	EXPECT_EQ(receiver.errmesg.empty(), true);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(receiver.recv_item, Pipe::log::item{});
}

TESTCASE(Pipe_json_log_reader_read_first_item_not_an_object)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;

	std::string str{"[\"Some array\"]"};
	str += to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the first message"
			}
		)
	);

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{str}));

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);
	REQUIRE_EQ(receiver.errmesg.size(), 1);
	EXPECT_EQ(receiver.errmesg.front(), "A log item must be an object");
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(
		receiver.recv_item,
		(Pipe::log::item{
			.when = {},
			.severity = {},
			.message = "This is the first message"
		})
	);
}

TESTCASE(Pipe_json_log_reader_read_first_item_not_valid)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;

	std::string str{"{\"Some array\":[]}"};
	str += to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the first message"
			}
		)
	);

	fcntl(logpipe.read_end().native_handle(), F_SETFL, O_NONBLOCK);
	write(logpipe.write_end(), std::as_bytes(std::span{str}));

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, false);
	REQUIRE_EQ(receiver.errmesg.size(), 1);
	EXPECT_EQ(
		receiver.errmesg.front(),
		"Failed to extract mandatory field `when` from received log item"
	);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::completed);
	EXPECT_EQ(
		receiver.recv_item,
		(Pipe::log::item{
			.when = {},
			.severity = {},
			.message = "This is the first message"
		})
	);
}

TESTCASE(Pipe_json_log_reader_read_jammed_parser)
{
	my_receiver receiver;
	Pipe::json_log::reader reader{"foo", std::ref(receiver)};
	Pipe::os_services::ipc::pipe logpipe;

	std::string str{"This is definitely not valid JSON"};
	str += to_string(
		Pipe::json_log::to_jopp_object(
			Pipe::log::item{
				.when = {},
				.severity = {},
				.message = "This is the first message"
			}
		)
	);

	write(logpipe.write_end(), std::as_bytes(std::span{str}));

	auto listening_status = Pipe::os_services::fd::activity_status::read;
	bool stop_listening = false;

	reader.handle_event(
		my_fd_activity_event{
			Pipe::os_services::fd::activity_status::read,
			&listening_status,
			&stop_listening
		},
		logpipe.read_end()
	);

	EXPECT_EQ(
		listening_status,
		Pipe::os_services::fd::activity_status::read
	);
	EXPECT_EQ(stop_listening, true);
	EXPECT_EQ(receiver.errmesg.size(), 0);
	EXPECT_EQ(receiver.parser_error, jopp::parser_error_code::no_top_level_node);
	EXPECT_EQ(receiver.recv_item, Pipe::log::item{});
}