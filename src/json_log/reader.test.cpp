//@	{"target":{"name": "reader.test"}}

#include "./reader.hpp"
#include "src/log/log.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/os_services/ipc/pipe.hpp"

#include <jopp/parser.hpp>
#include <testfwk/testfwk.hpp>

namespace
{
	struct my_receiver
	{
		Pipe::log::item recv_item{};
		jopp::parser_error_code parser_error = jopp::parser_error_code::completed;
		std::string errmesg{};

		void consume(Pipe::log::item&& item)
		{ recv_item = std::move(item); }

		void on_parse_error(jopp::parser_error_code ec)
		{ parser_error = ec; }

		void on_invalid_log_item(char const* msg)
		{ errmesg = msg;}
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
	Pipe::json_log::reader reader{16, std::ref(receiver)};
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